# Testing ABI Files

> 출처(원문): https://docs.kernel.org/admin-guide/abi-testing-files.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Testing ABI Files

## ABI file testing/configfs-acpi

Has the following ABI:

* [/config/acpi](abi-testing.html#abi-config-acpi)
* [/config/acpi/table](abi-testing.html#abi-config-acpi-table)

## ABI file testing/configfs-iio

Has the following ABI:

* [/config/iio](abi-testing.html#abi-config-iio)
* [/config/iio/triggers](abi-testing.html#abi-config-iio-triggers)
* [/config/iio/triggers/hrtimers](abi-testing.html#abi-config-iio-triggers-hrtimers)
* [/config/iio/devices](abi-testing.html#abi-config-iio-devices)
* [/config/iio/devices/dummy](abi-testing.html#abi-config-iio-devices-dummy)

## ABI file testing/configfs-most

Has the following ABI:

* [/sys/kernel/config/most\_<component>](abi-testing.html#abi-sys-kernel-config-most-component)
* [/sys/kernel/config/most\_cdev/<link>](abi-testing.html#abi-sys-kernel-config-most-cdev-link)
* [/sys/kernel/config/most\_video/<link>](abi-testing.html#abi-sys-kernel-config-most-video-link)
* [/sys/kernel/config/most\_net/<link>](abi-testing.html#abi-sys-kernel-config-most-net-link)
* [/sys/kernel/config/most\_sound/<card>](abi-testing.html#abi-sys-kernel-config-most-sound-card)
* [/sys/kernel/config/most\_sound/<card>/<link>](abi-testing.html#abi-sys-kernel-config-most-sound-card-link)

## ABI file testing/configfs-rdma\_cm

Has the following ABI:

* [/config/rdma\_cm](abi-testing.html#abi-config-rdma-cm)
* [/config/rdma\_cm/<hca>/ports/<port-num>/default\_roce\_mode](abi-testing.html#abi-config-rdma-cm-hca-ports-port-num-default-roce-mode)
* [/config/rdma\_cm/<hca>/ports/<port-num>/default\_roce\_tos](abi-testing.html#abi-config-rdma-cm-hca-ports-port-num-default-roce-tos)

## ABI file testing/configfs-spear-pcie-gadget

Has the following ABI:

* [/config/pcie-gadget](abi-testing.html#abi-config-pcie-gadget)

## ABI file testing/configfs-stp-policy

Has the following ABI:

* [/config/stp-policy](abi-testing.html#abi-config-stp-policy)
* [/config/stp-policy/<device>.<policy>](abi-testing.html#abi-config-stp-policy-device-policy)
* [/config/stp-policy/<device>.<policy>/device](abi-testing.html#abi-config-stp-policy-device-policy-device)
* [/config/stp-policy/<device>.<policy>/<node>](abi-testing.html#abi-config-stp-policy-device-policy-node)
* [/config/stp-policy/<device>.<policy>/<node>/masters](abi-testing.html#abi-config-stp-policy-device-policy-node-masters)
* [/config/stp-policy/<device>.<policy>/<node>/channels](abi-testing.html#abi-config-stp-policy-device-policy-node-channels)

## ABI file testing/configfs-stp-policy-p\_sys-t

Has the following ABI:

* [/config/stp-policy/<device>:p\_sys-t.<policy>/<node>/uuid](abi-testing.html#abi-config-stp-policy-device-p-sys-t-policy-node-uuid)
* [/config/stp-policy/<device>:p\_sys-t.<policy>/<node>/do\_len](abi-testing.html#abi-config-stp-policy-device-p-sys-t-policy-node-do-len)
* [/config/stp-policy/<device>:p\_sys-t.<policy>/<node>/ts\_interval](abi-testing.html#abi-config-stp-policy-device-p-sys-t-policy-node-ts-interval)
* [/config/stp-policy/<device>:p\_sys-t.<policy>/<node>/clocksync\_interval](abi-testing.html#abi-config-stp-policy-device-p-sys-t-policy-node-clocksync-interval)

## ABI file testing/configfs-tsm-report

Has the following ABI:

* [/sys/kernel/config/tsm/report/$name/inblob](abi-testing.html#abi-sys-kernel-config-tsm-report-name-inblob)
* [/sys/kernel/config/tsm/report/$name/outblob](abi-testing.html#abi-sys-kernel-config-tsm-report-name-outblob)
* [/sys/kernel/config/tsm/report/$name/auxblob](abi-testing.html#abi-sys-kernel-config-tsm-report-name-auxblob)
* [/sys/kernel/config/tsm/report/$name/manifestblob](abi-testing.html#abi-sys-kernel-config-tsm-report-name-manifestblob)
* [/sys/kernel/config/tsm/report/$name/provider](abi-testing.html#abi-sys-kernel-config-tsm-report-name-provider)
* [/sys/kernel/config/tsm/report/$name/generation](abi-testing.html#abi-sys-kernel-config-tsm-report-name-generation)
* [/sys/kernel/config/tsm/report/$name/privlevel](abi-testing.html#abi-sys-kernel-config-tsm-report-name-privlevel)
* [/sys/kernel/config/tsm/report/$name/privlevel\_floor](abi-testing.html#abi-sys-kernel-config-tsm-report-name-privlevel-floor)
* [/sys/kernel/config/tsm/report/$name/service\_provider](abi-testing.html#abi-sys-kernel-config-tsm-report-name-service-provider)
* [/sys/kernel/config/tsm/report/$name/service\_guid](abi-testing.html#abi-sys-kernel-config-tsm-report-name-service-guid)
* [/sys/kernel/config/tsm/report/$name/service\_manifest\_version](abi-testing.html#abi-sys-kernel-config-tsm-report-name-service-manifest-version)

## ABI file testing/configfs-usb-gadget

Has the following ABI:

* [/config/usb-gadget](abi-testing.html#abi-config-usb-gadget)
* [/config/usb-gadget/gadget](abi-testing.html#abi-config-usb-gadget-gadget)
* [/config/usb-gadget/gadget/configs](abi-testing.html#abi-config-usb-gadget-gadget-configs)
* [/config/usb-gadget/gadget/configs/config](abi-testing.html#abi-config-usb-gadget-gadget-configs-config)
* [/config/usb-gadget/gadget/configs/config/strings](abi-testing.html#abi-config-usb-gadget-gadget-configs-config-strings)
* [/config/usb-gadget/gadget/configs/config/strings/language](abi-testing.html#abi-config-usb-gadget-gadget-configs-config-strings-language)
* [/config/usb-gadget/gadget/functions](abi-testing.html#abi-config-usb-gadget-gadget-functions)
* [/config/usb-gadget/gadget/functions/<func>.<inst>/interface.<n>](abi-testing.html#abi-config-usb-gadget-gadget-functions-func-inst-interface-n)
* [/config/usb-gadget/gadget/functions/<func>.<inst>/interface.<n>/<property>](abi-testing.html#abi-config-usb-gadget-gadget-functions-func-inst-interface-n-property)
* [/config/usb-gadget/gadget/strings](abi-testing.html#abi-config-usb-gadget-gadget-strings)
* [/config/usb-gadget/gadget/strings/language](abi-testing.html#abi-config-usb-gadget-gadget-strings-language)
* [/config/usb-gadget/gadget/os\_desc](abi-testing.html#abi-config-usb-gadget-gadget-os-desc)
* [/config/usb-gadget/gadget/webusb](abi-testing.html#abi-config-usb-gadget-gadget-webusb)

## ABI file testing/configfs-usb-gadget-acm

Has the following ABI:

* [/config/usb-gadget/gadget/functions/acm.name](abi-testing.html#abi-config-usb-gadget-gadget-functions-acm-name)
* [/config/usb-gadget/gadget/functions/acm.name/protocol](abi-testing.html#abi-config-usb-gadget-gadget-functions-acm-name-protocol)

## ABI file testing/configfs-usb-gadget-ecm

Has the following ABI:

* [/config/usb-gadget/gadget/functions/ecm.name](abi-testing.html#abi-config-usb-gadget-gadget-functions-ecm-name)

## ABI file testing/configfs-usb-gadget-eem

Has the following ABI:

* [/config/usb-gadget/gadget/functions/eem.name](abi-testing.html#abi-config-usb-gadget-gadget-functions-eem-name)

## ABI file testing/configfs-usb-gadget-ffs

Has the following ABI:

* [/config/usb-gadget/gadget/functions/ffs.name](abi-testing.html#abi-config-usb-gadget-gadget-functions-ffs-name)

## ABI file testing/configfs-usb-gadget-hid

Has the following ABI:

* [/config/usb-gadget/gadget/functions/hid.name](abi-testing.html#abi-config-usb-gadget-gadget-functions-hid-name)

## ABI file testing/configfs-usb-gadget-loopback

Has the following ABI:

* [/config/usb-gadget/gadget/functions/Loopback.name](abi-testing.html#abi-config-usb-gadget-gadget-functions-loopback-name)

## ABI file testing/configfs-usb-gadget-mass-storage

Has the following ABI:

* [/config/usb-gadget/gadget/functions/mass\_storage.name](abi-testing.html#abi-config-usb-gadget-gadget-functions-mass-storage-name)
* [/config/usb-gadget/gadget/functions/mass\_storage.name/lun.name](abi-testing.html#abi-config-usb-gadget-gadget-functions-mass-storage-name-lun-name)

## ABI file testing/configfs-usb-gadget-midi

Has the following ABI:

* [/config/usb-gadget/gadget/functions/midi.name](abi-testing.html#abi-config-usb-gadget-gadget-functions-midi-name)

## ABI file testing/configfs-usb-gadget-midi2

Has the following ABI:

* [/config/usb-gadget/gadget/functions/midi2.name](abi-testing.html#abi-config-usb-gadget-gadget-functions-midi2-name)
* [/config/usb-gadget/gadget/functions/midi2.name/ep.number](abi-testing.html#abi-config-usb-gadget-gadget-functions-midi2-name-ep-number)
* [/config/usb-gadget/gadget/functions/midi2.name/ep.number/block.number](abi-testing.html#abi-config-usb-gadget-gadget-functions-midi2-name-ep-number-block-number)

## ABI file testing/configfs-usb-gadget-ncm

Has the following ABI:

* [/config/usb-gadget/gadget/functions/ncm.name](abi-testing.html#abi-config-usb-gadget-gadget-functions-ncm-name)

## ABI file testing/configfs-usb-gadget-obex

Has the following ABI:

* [/config/usb-gadget/gadget/functions/obex.name](abi-testing.html#abi-config-usb-gadget-gadget-functions-obex-name)

## ABI file testing/configfs-usb-gadget-phonet

Has the following ABI:

* [/config/usb-gadget/gadget/functions/phonet.name](abi-testing.html#abi-config-usb-gadget-gadget-functions-phonet-name)

## ABI file testing/configfs-usb-gadget-printer

Has the following ABI:

* [/config/usb-gadget/gadget/functions/printer.name](abi-testing.html#abi-config-usb-gadget-gadget-functions-printer-name)

## ABI file testing/configfs-usb-gadget-rndis

Has the following ABI:

* [/config/usb-gadget/gadget/functions/rndis.name](abi-testing.html#abi-config-usb-gadget-gadget-functions-rndis-name)

## ABI file testing/configfs-usb-gadget-serial

Has the following ABI:

* [/config/usb-gadget/gadget/functions/gser.name](abi-testing.html#abi-config-usb-gadget-gadget-functions-gser-name)

## ABI file testing/configfs-usb-gadget-sourcesink

Has the following ABI:

* [/config/usb-gadget/gadget/functions/SourceSink.name](abi-testing.html#abi-config-usb-gadget-gadget-functions-sourcesink-name)

## ABI file testing/configfs-usb-gadget-subset

Has the following ABI:

* [/config/usb-gadget/gadget/functions/geth.name](abi-testing.html#abi-config-usb-gadget-gadget-functions-geth-name)

## ABI file testing/configfs-usb-gadget-tcm

Has the following ABI:

* [/config/usb-gadget/gadget/functions/tcm.name](abi-testing.html#abi-config-usb-gadget-gadget-functions-tcm-name)

## ABI file testing/configfs-usb-gadget-uac1

Has the following ABI:

* [/config/usb-gadget/gadget/functions/uac1.name](abi-testing.html#abi-config-usb-gadget-gadget-functions-uac1-name)

## ABI file testing/configfs-usb-gadget-uac1\_legacy

Has the following ABI:

* [/config/usb-gadget/gadget/functions/uac1\_legacy.name](abi-testing.html#abi-config-usb-gadget-gadget-functions-uac1-legacy-name)

## ABI file testing/configfs-usb-gadget-uac2

Has the following ABI:

* [/config/usb-gadget/gadget/functions/uac2.name](abi-testing.html#abi-config-usb-gadget-gadget-functions-uac2-name)

## ABI file testing/configfs-usb-gadget-uvc

Has the following ABI:

* [/config/usb-gadget/gadget/functions/uvc.name](abi-testing.html#abi-config-usb-gadget-gadget-functions-uvc-name)
* [/config/usb-gadget/gadget/functions/uvc.name/control](abi-testing.html#abi-config-usb-gadget-gadget-functions-uvc-name-control)
* [/config/usb-gadget/gadget/functions/uvc.name/control/class](abi-testing.html#abi-config-usb-gadget-gadget-functions-uvc-name-control-class)
* [/config/usb-gadget/gadget/functions/uvc.name/control/class/ss](abi-testing.html#abi-config-usb-gadget-gadget-functions-uvc-name-control-class-ss)
* [/config/usb-gadget/gadget/functions/uvc.name/control/class/fs](abi-testing.html#abi-config-usb-gadget-gadget-functions-uvc-name-control-class-fs)
* [/config/usb-gadget/gadget/functions/uvc.name/control/terminal](abi-testing.html#abi-config-usb-gadget-gadget-functions-uvc-name-control-terminal)
* [/config/usb-gadget/gadget/functions/uvc.name/control/terminal/output](abi-testing.html#abi-config-usb-gadget-gadget-functions-uvc-name-control-terminal-output)
* [/config/usb-gadget/gadget/functions/uvc.name/control/terminal/output/default](abi-testing.html#abi-config-usb-gadget-gadget-functions-uvc-name-control-terminal-output-default)
* [/config/usb-gadget/gadget/functions/uvc.name/control/terminal/camera](abi-testing.html#abi-config-usb-gadget-gadget-functions-uvc-name-control-terminal-camera)
* [/config/usb-gadget/gadget/functions/uvc.name/control/terminal/camera/default](abi-testing.html#abi-config-usb-gadget-gadget-functions-uvc-name-control-terminal-camera-default)
* [/config/usb-gadget/gadget/functions/uvc.name/control/processing](abi-testing.html#abi-config-usb-gadget-gadget-functions-uvc-name-control-processing)
* [/config/usb-gadget/gadget/functions/uvc.name/control/processing/default](abi-testing.html#abi-config-usb-gadget-gadget-functions-uvc-name-control-processing-default)
* [/config/usb-gadget/gadget/functions/uvc.name/control/extensions](abi-testing.html#abi-config-usb-gadget-gadget-functions-uvc-name-control-extensions)
* [/config/usb-gadget/gadget/functions/uvc.name/control/extensions/name](abi-testing.html#abi-config-usb-gadget-gadget-functions-uvc-name-control-extensions-name)
* [/config/usb-gadget/gadget/functions/uvc.name/control/header](abi-testing.html#abi-config-usb-gadget-gadget-functions-uvc-name-control-header)
* [/config/usb-gadget/gadget/functions/uvc.name/control/header/name](abi-testing.html#abi-config-usb-gadget-gadget-functions-uvc-name-control-header-name)
* [/config/usb-gadget/gadget/functions/uvc.name/streaming](abi-testing.html#abi-config-usb-gadget-gadget-functions-uvc-name-streaming)
* [/config/usb-gadget/gadget/functions/uvc.name/streaming/class](abi-testing.html#abi-config-usb-gadget-gadget-functions-uvc-name-streaming-class)
* [/config/usb-gadget/gadget/functions/uvc.name/streaming/class/ss](abi-testing.html#abi-config-usb-gadget-gadget-functions-uvc-name-streaming-class-ss)
* [/config/usb-gadget/gadget/functions/uvc.name/streaming/class/hs](abi-testing.html#abi-config-usb-gadget-gadget-functions-uvc-name-streaming-class-hs)
* [/config/usb-gadget/gadget/functions/uvc.name/streaming/class/fs](abi-testing.html#abi-config-usb-gadget-gadget-functions-uvc-name-streaming-class-fs)
* [/config/usb-gadget/gadget/functions/uvc.name/streaming/color\_matching](abi-testing.html#abi-config-usb-gadget-gadget-functions-uvc-name-streaming-color-matching)
* [/config/usb-gadget/gadget/functions/uvc.name/streaming/color\_matching/default](abi-testing.html#abi-config-usb-gadget-gadget-functions-uvc-name-streaming-color-matching-default)
* [/config/usb-gadget/gadget/functions/uvc.name/streaming/color\_matching/name](abi-testing.html#abi-config-usb-gadget-gadget-functions-uvc-name-streaming-color-matching-name)
* [/config/usb-gadget/gadget/functions/uvc.name/streaming/mjpeg](abi-testing.html#abi-config-usb-gadget-gadget-functions-uvc-name-streaming-mjpeg)
* [/config/usb-gadget/gadget/functions/uvc.name/streaming/mjpeg/name](abi-testing.html#abi-config-usb-gadget-gadget-functions-uvc-name-streaming-mjpeg-name)
* [/config/usb-gadget/gadget/functions/uvc.name/streaming/mjpeg/name/name](abi-testing.html#abi-config-usb-gadget-gadget-functions-uvc-name-streaming-mjpeg-name-name)
* [/config/usb-gadget/gadget/functions/uvc.name/streaming/uncompressed](abi-testing.html#abi-config-usb-gadget-gadget-functions-uvc-name-streaming-uncompressed)
* [/config/usb-gadget/gadget/functions/uvc.name/streaming/uncompressed/name](abi-testing.html#abi-config-usb-gadget-gadget-functions-uvc-name-streaming-uncompressed-name)
* [/config/usb-gadget/gadget/functions/uvc.name/streaming/uncompressed/name/name](abi-testing.html#abi-config-usb-gadget-gadget-functions-uvc-name-streaming-uncompressed-name-name)
* [/config/usb-gadget/gadget/functions/uvc.name/streaming/framebased](abi-testing.html#abi-config-usb-gadget-gadget-functions-uvc-name-streaming-framebased)
* [/config/usb-gadget/gadget/functions/uvc.name/streaming/framebased/name](abi-testing.html#abi-config-usb-gadget-gadget-functions-uvc-name-streaming-framebased-name)
* [/config/usb-gadget/gadget/functions/uvc.name/streaming/framebased/name/name](abi-testing.html#abi-config-usb-gadget-gadget-functions-uvc-name-streaming-framebased-name-name)
* [/config/usb-gadget/gadget/functions/uvc.name/streaming/header](abi-testing.html#abi-config-usb-gadget-gadget-functions-uvc-name-streaming-header)
* [/config/usb-gadget/gadget/functions/uvc.name/streaming/header/name](abi-testing.html#abi-config-usb-gadget-gadget-functions-uvc-name-streaming-header-name)
* [/sys/class/udc/udc.name/device/gadget/video4linux/video.name/function\_name](abi-testing.html#abi-sys-class-udc-udc-name-device-gadget-video4linux-video-name-function-name)

## ABI file testing/debugfs-alienware-wmi

Has the following ABI:

* [/sys/kernel/debug/alienware-wmi-<wmi\_device\_name>/system\_description](abi-testing.html#abi-sys-kernel-debug-alienware-wmi-wmi-device-name-system-description)
* [/sys/kernel/debug/alienware-wmi-<wmi\_device\_name>/hwmon\_data](abi-testing.html#abi-sys-kernel-debug-alienware-wmi-wmi-device-name-hwmon-data)
* [/sys/kernel/debug/alienware-wmi-<wmi\_device\_name>/pprof\_data](abi-testing.html#abi-sys-kernel-debug-alienware-wmi-wmi-device-name-pprof-data)
* [/sys/kernel/debug/alienware-wmi-<wmi\_device\_name>/gpio\_ctl/total\_gpios](abi-testing.html#abi-sys-kernel-debug-alienware-wmi-wmi-device-name-gpio-ctl-total-gpios)
* [/sys/kernel/debug/alienware-wmi-<wmi\_device\_name>/gpio\_ctl/pinX](abi-testing.html#abi-sys-kernel-debug-alienware-wmi-wmi-device-name-gpio-ctl-pinx)

## ABI file testing/debugfs-amd-iommu

Has the following ABI:

* [/sys/kernel/debug/iommu/amd/iommu<x>/mmio](abi-testing.html#abi-sys-kernel-debug-iommu-amd-iommu-x-mmio)
* [/sys/kernel/debug/iommu/amd/iommu<x>/capability](abi-testing.html#abi-sys-kernel-debug-iommu-amd-iommu-x-capability)
* [/sys/kernel/debug/iommu/amd/iommu<x>/cmdbuf](abi-testing.html#abi-sys-kernel-debug-iommu-amd-iommu-x-cmdbuf)
* [/sys/kernel/debug/iommu/amd/devid](abi-testing.html#abi-sys-kernel-debug-iommu-amd-devid)
* [/sys/kernel/debug/iommu/amd/devtbl](abi-testing.html#abi-sys-kernel-debug-iommu-amd-devtbl)
* [/sys/kernel/debug/iommu/amd/irqtbl](abi-testing.html#abi-sys-kernel-debug-iommu-amd-irqtbl)

## ABI file testing/debugfs-cec-error-inj

Has the following ABI:

* [/sys/kernel/debug/cec/\*/error-inj](abi-testing.html#abi-sys-kernel-debug-cec-error-inj)

## ABI file testing/debugfs-cros-ec

Has the following ABI:

* [/sys/kernel/debug/<cros-ec-device>/console\_log](abi-testing.html#abi-sys-kernel-debug-cros-ec-device-console-log)
* [/sys/kernel/debug/<cros-ec-device>/panicinfo](abi-testing.html#abi-sys-kernel-debug-cros-ec-device-panicinfo)
* [/sys/kernel/debug/<cros-ec-device>/pdinfo](abi-testing.html#abi-sys-kernel-debug-cros-ec-device-pdinfo)
* [/sys/kernel/debug/<cros-ec-device>/uptime](abi-testing.html#abi-sys-kernel-debug-cros-ec-device-uptime)
* [/sys/kernel/debug/<cros-ec-device>/last\_resume\_result](abi-testing.html#abi-sys-kernel-debug-cros-ec-device-last-resume-result)
* [/sys/kernel/debug/<cros-ec-device>/suspend\_timeout\_ms](abi-testing.html#abi-sys-kernel-debug-cros-ec-device-suspend-timeout-ms)

## ABI file testing/debugfs-cxl

Has the following ABI:

* [/sys/kernel/debug/cxl/memX/inject\_poison](abi-testing.html#abi-sys-kernel-debug-cxl-memx-inject-poison)
* [/sys/kernel/debug/cxl/memX/clear\_poison](abi-testing.html#abi-sys-kernel-debug-cxl-memx-clear-poison)
* [/sys/kernel/debug/cxl/regionX/inject\_poison](abi-testing.html#abi-sys-kernel-debug-cxl-regionx-inject-poison)
* [/sys/kernel/debug/cxl/regionX/clear\_poison](abi-testing.html#abi-sys-kernel-debug-cxl-regionx-clear-poison)
* [/sys/kernel/debug/cxl/einj\_types](abi-testing.html#abi-sys-kernel-debug-cxl-einj-types)
* [/sys/kernel/debug/cxl/$dport\_dev/einj\_inject](abi-testing.html#abi-sys-kernel-debug-cxl-dport-dev-einj-inject)

## ABI file testing/debugfs-dell-wmi-ddv

Has the following ABI:

* [/sys/kernel/debug/dell-wmi-ddv-<wmi\_device\_name>/fan\_sensor\_information](abi-testing.html#abi-sys-kernel-debug-dell-wmi-ddv-wmi-device-name-fan-sensor-information)
* [/sys/kernel/debug/dell-wmi-ddv-<wmi\_device\_name>/thermal\_sensor\_information](abi-testing.html#abi-sys-kernel-debug-dell-wmi-ddv-wmi-device-name-thermal-sensor-information)

## ABI file testing/debugfs-driver-dcc

Has the following ABI:

* [/sys/kernel/debug/dcc/.../ready](abi-testing.html#abi-sys-kernel-debug-dcc-ready)
* [/sys/kernel/debug/dcc/.../trigger](abi-testing.html#abi-sys-kernel-debug-dcc-trigger)
* [/sys/kernel/debug/dcc/.../config\_reset](abi-testing.html#abi-sys-kernel-debug-dcc-config-reset)
* [/sys/kernel/debug/dcc/.../[list-number]/config](abi-testing.html#abi-sys-kernel-debug-dcc-list-number-config)
* [/sys/kernel/debug/dcc/.../[list-number]/enable](abi-testing.html#abi-sys-kernel-debug-dcc-list-number-enable)

## ABI file testing/debugfs-driver-genwqe

Has the following ABI:

* [/sys/kernel/debug/genwqe/genwqe<n>\_card/ddcb\_info](abi-testing.html#abi-sys-kernel-debug-genwqe-genwqe-n-card-ddcb-info)
* [/sys/kernel/debug/genwqe/genwqe<n>\_card/curr\_regs](abi-testing.html#abi-sys-kernel-debug-genwqe-genwqe-n-card-curr-regs)
* [/sys/kernel/debug/genwqe/genwqe<n>\_card/curr\_dbg\_uid0](abi-testing.html#abi-sys-kernel-debug-genwqe-genwqe-n-card-curr-dbg-uid0)
* [/sys/kernel/debug/genwqe/genwqe<n>\_card/curr\_dbg\_uid1](abi-testing.html#abi-sys-kernel-debug-genwqe-genwqe-n-card-curr-dbg-uid1)
* [/sys/kernel/debug/genwqe/genwqe<n>\_card/curr\_dbg\_uid2](abi-testing.html#abi-sys-kernel-debug-genwqe-genwqe-n-card-curr-dbg-uid2)
* [/sys/kernel/debug/genwqe/genwqe<n>\_card/prev\_regs](abi-testing.html#abi-sys-kernel-debug-genwqe-genwqe-n-card-prev-regs)
* [/sys/kernel/debug/genwqe/genwqe<n>\_card/prev\_dbg\_uid0](abi-testing.html#abi-sys-kernel-debug-genwqe-genwqe-n-card-prev-dbg-uid0)
* [/sys/kernel/debug/genwqe/genwqe<n>\_card/prev\_dbg\_uid1](abi-testing.html#abi-sys-kernel-debug-genwqe-genwqe-n-card-prev-dbg-uid1)
* [/sys/kernel/debug/genwqe/genwqe<n>\_card/prev\_dbg\_uid2](abi-testing.html#abi-sys-kernel-debug-genwqe-genwqe-n-card-prev-dbg-uid2)
* [/sys/kernel/debug/genwqe/genwqe<n>\_card/info](abi-testing.html#abi-sys-kernel-debug-genwqe-genwqe-n-card-info)
* [/sys/kernel/debug/genwqe/genwqe<n>\_card/err\_inject](abi-testing.html#abi-sys-kernel-debug-genwqe-genwqe-n-card-err-inject)
* [/sys/kernel/debug/genwqe/genwqe<n>\_card/vf<0..14>\_jobtimeout\_msec](abi-testing.html#abi-sys-kernel-debug-genwqe-genwqe-n-card-vf-0-14-jobtimeout-msec)
* [/sys/kernel/debug/genwqe/genwqe<n>\_card/jobtimer](abi-testing.html#abi-sys-kernel-debug-genwqe-genwqe-n-card-jobtimer)
* [/sys/kernel/debug/genwqe/genwqe<n>\_card/queue\_working\_time](abi-testing.html#abi-sys-kernel-debug-genwqe-genwqe-n-card-queue-working-time)

## ABI file testing/debugfs-driver-habanalabs

Has the following ABI:

* [/sys/kernel/debug/accel/<parent\_device>/addr](abi-testing.html#abi-sys-kernel-debug-accel-parent-device-addr)
* [/sys/kernel/debug/accel/<parent\_device>/clk\_gate](abi-testing.html#abi-sys-kernel-debug-accel-parent-device-clk-gate)
* [/sys/kernel/debug/accel/<parent\_device>/command\_buffers](abi-testing.html#abi-sys-kernel-debug-accel-parent-device-command-buffers)
* [/sys/kernel/debug/accel/<parent\_device>/command\_submission](abi-testing.html#abi-sys-kernel-debug-accel-parent-device-command-submission)
* [/sys/kernel/debug/accel/<parent\_device>/command\_submission\_jobs](abi-testing.html#abi-sys-kernel-debug-accel-parent-device-command-submission-jobs)
* [/sys/kernel/debug/accel/<parent\_device>/data32](abi-testing.html#abi-sys-kernel-debug-accel-parent-device-data32)
* [/sys/kernel/debug/accel/<parent\_device>/data64](abi-testing.html#abi-sys-kernel-debug-accel-parent-device-data64)
* [/sys/kernel/debug/accel/<parent\_device>/data\_dma](abi-testing.html#abi-sys-kernel-debug-accel-parent-device-data-dma)
* [/sys/kernel/debug/accel/<parent\_device>/device](abi-testing.html#abi-sys-kernel-debug-accel-parent-device-device)
* [/sys/kernel/debug/accel/<parent\_device>/device\_release\_watchdog\_timeout](abi-testing.html#abi-sys-kernel-debug-accel-parent-device-device-release-watchdog-timeout)
* [/sys/kernel/debug/accel/<parent\_device>/dma\_size](abi-testing.html#abi-sys-kernel-debug-accel-parent-device-dma-size)
* [/sys/kernel/debug/accel/<parent\_device>/dump\_razwi\_events](abi-testing.html#abi-sys-kernel-debug-accel-parent-device-dump-razwi-events)
* [/sys/kernel/debug/accel/<parent\_device>/dump\_security\_violations](abi-testing.html#abi-sys-kernel-debug-accel-parent-device-dump-security-violations)
* [/sys/kernel/debug/accel/<parent\_device>/engines](abi-testing.html#abi-sys-kernel-debug-accel-parent-device-engines)
* [/sys/kernel/debug/accel/<parent\_device>/i2c\_addr](abi-testing.html#abi-sys-kernel-debug-accel-parent-device-i2c-addr)
* [/sys/kernel/debug/accel/<parent\_device>/i2c\_bus](abi-testing.html#abi-sys-kernel-debug-accel-parent-device-i2c-bus)
* [/sys/kernel/debug/accel/<parent\_device>/i2c\_data](abi-testing.html#abi-sys-kernel-debug-accel-parent-device-i2c-data)
* [/sys/kernel/debug/accel/<parent\_device>/i2c\_len](abi-testing.html#abi-sys-kernel-debug-accel-parent-device-i2c-len)
* [/sys/kernel/debug/accel/<parent\_device>/i2c\_reg](abi-testing.html#abi-sys-kernel-debug-accel-parent-device-i2c-reg)
* [/sys/kernel/debug/accel/<parent\_device>/led0](abi-testing.html#abi-sys-kernel-debug-accel-parent-device-led0)
* [/sys/kernel/debug/accel/<parent\_device>/led1](abi-testing.html#abi-sys-kernel-debug-accel-parent-device-led1)
* [/sys/kernel/debug/accel/<parent\_device>/led2](abi-testing.html#abi-sys-kernel-debug-accel-parent-device-led2)
* [/sys/kernel/debug/accel/<parent\_device>/memory\_scrub](abi-testing.html#abi-sys-kernel-debug-accel-parent-device-memory-scrub)
* [/sys/kernel/debug/accel/<parent\_device>/memory\_scrub\_val](abi-testing.html#abi-sys-kernel-debug-accel-parent-device-memory-scrub-val)
* [/sys/kernel/debug/accel/<parent\_device>/mmu](abi-testing.html#abi-sys-kernel-debug-accel-parent-device-mmu)
* [/sys/kernel/debug/accel/<parent\_device>/mmu\_error](abi-testing.html#abi-sys-kernel-debug-accel-parent-device-mmu-error)
* [/sys/kernel/debug/accel/<parent\_device>/monitor\_dump](abi-testing.html#abi-sys-kernel-debug-accel-parent-device-monitor-dump)
* [/sys/kernel/debug/accel/<parent\_device>/monitor\_dump\_trig](abi-testing.html#abi-sys-kernel-debug-accel-parent-device-monitor-dump-trig)
* [/sys/kernel/debug/accel/<parent\_device>/server\_type](abi-testing.html#abi-sys-kernel-debug-accel-parent-device-server-type)
* [/sys/kernel/debug/accel/<parent\_device>/set\_power\_state](abi-testing.html#abi-sys-kernel-debug-accel-parent-device-set-power-state)
* [/sys/kernel/debug/accel/<parent\_device>/skip\_reset\_on\_timeout](abi-testing.html#abi-sys-kernel-debug-accel-parent-device-skip-reset-on-timeout)
* [/sys/kernel/debug/accel/<parent\_device>/state\_dump](abi-testing.html#abi-sys-kernel-debug-accel-parent-device-state-dump)
* [/sys/kernel/debug/accel/<parent\_device>/stop\_on\_err](abi-testing.html#abi-sys-kernel-debug-accel-parent-device-stop-on-err)
* [/sys/kernel/debug/accel/<parent\_device>/timeout\_locked](abi-testing.html#abi-sys-kernel-debug-accel-parent-device-timeout-locked)
* [/sys/kernel/debug/accel/<parent\_device>/userptr](abi-testing.html#abi-sys-kernel-debug-accel-parent-device-userptr)
* [/sys/kernel/debug/accel/<parent\_device>/userptr\_lookup](abi-testing.html#abi-sys-kernel-debug-accel-parent-device-userptr-lookup)
* [/sys/kernel/debug/accel/<parent\_device>/vm](abi-testing.html#abi-sys-kernel-debug-accel-parent-device-vm)

## ABI file testing/debugfs-driver-qat

Has the following ABI:

* [/sys/kernel/debug/qat\_<device>\_<BDF>/fw\_counters](abi-testing.html#abi-sys-kernel-debug-qat-device-bdf-fw-counters)
* [/sys/kernel/debug/qat\_<device>\_<BDF>/heartbeat/config](abi-testing.html#abi-sys-kernel-debug-qat-device-bdf-heartbeat-config)
* [/sys/kernel/debug/qat\_<device>\_<BDF>/heartbeat/queries\_failed](abi-testing.html#abi-sys-kernel-debug-qat-device-bdf-heartbeat-queries-failed)
* [/sys/kernel/debug/qat\_<device>\_<BDF>/heartbeat/queries\_sent](abi-testing.html#abi-sys-kernel-debug-qat-device-bdf-heartbeat-queries-sent)
* [/sys/kernel/debug/qat\_<device>\_<BDF>/heartbeat/status](abi-testing.html#abi-sys-kernel-debug-qat-device-bdf-heartbeat-status)
* [/sys/kernel/debug/qat\_<device>\_<BDF>/pm\_status](abi-testing.html#abi-sys-kernel-debug-qat-device-bdf-pm-status)
* [/sys/kernel/debug/qat\_<device>\_<BDF>/cnv\_errors](abi-testing.html#abi-sys-kernel-debug-qat-device-bdf-cnv-errors)
* [/sys/kernel/debug/qat\_<device>\_<BDF>/heartbeat/inject\_error](abi-testing.html#abi-sys-kernel-debug-qat-device-bdf-heartbeat-inject-error)

## ABI file testing/debugfs-driver-qat\_telemetry

Has the following ABI:

* [/sys/kernel/debug/qat\_<device>\_<BDF>/telemetry/control](abi-testing.html#abi-sys-kernel-debug-qat-device-bdf-telemetry-control)
* [/sys/kernel/debug/qat\_<device>\_<BDF>/telemetry/device\_data](abi-testing.html#abi-sys-kernel-debug-qat-device-bdf-telemetry-device-data)
* [/sys/kernel/debug/qat\_<device>\_<BDF>/telemetry/rp\_<A/B/C/D>\_data](abi-testing.html#abi-sys-kernel-debug-qat-device-bdf-telemetry-rp-a-b-c-d-data)

## ABI file testing/debugfs-dwc-pcie

Has the following ABI:

* [/sys/kernel/debug/dwc\_pcie\_<dev>/rasdes\_debug/lane\_detect](abi-testing.html#abi-sys-kernel-debug-dwc-pcie-dev-rasdes-debug-lane-detect)
* [/sys/kernel/debug/dwc\_pcie\_<dev>/rasdes\_debug/rx\_valid](abi-testing.html#abi-sys-kernel-debug-dwc-pcie-dev-rasdes-debug-rx-valid)
* [/sys/kernel/debug/dwc\_pcie\_<dev>/rasdes\_err\_inj/<error>](abi-testing.html#abi-sys-kernel-debug-dwc-pcie-dev-rasdes-err-inj-error)
* [/sys/kernel/debug/dwc\_pcie\_<dev>/rasdes\_event\_counters/<event>/counter\_enable](abi-testing.html#abi-sys-kernel-debug-dwc-pcie-dev-rasdes-event-counters-event-counter-enable)
* [/sys/kernel/debug/dwc\_pcie\_<dev>/rasdes\_event\_counters/<event>/counter\_value](abi-testing.html#abi-sys-kernel-debug-dwc-pcie-dev-rasdes-event-counters-event-counter-value)
* [/sys/kernel/debug/dwc\_pcie\_<dev>/rasdes\_event\_counters/<event>/lane\_select](abi-testing.html#abi-sys-kernel-debug-dwc-pcie-dev-rasdes-event-counters-event-lane-select)
* [/sys/kernel/debug/dwc\_pcie\_<dev>/ltssm\_status](abi-testing.html#abi-sys-kernel-debug-dwc-pcie-dev-ltssm-status)

## ABI file testing/debugfs-ec

Has the following ABI:

* [/sys/kernel/debug/ec/\*/{gpe,use\_global\_lock,io}](abi-testing.html#abi-sys-kernel-debug-ec-gpe-use-global-lock-io)

## ABI file testing/debugfs-hisi-hpre

Has the following ABI:

* [/sys/kernel/debug/hisi\_hpre/<bdf>/cluster[0-3]/regs](abi-testing.html#abi-sys-kernel-debug-hisi-hpre-bdf-cluster-0-3-regs)
* [/sys/kernel/debug/hisi\_hpre/<bdf>/cluster[0-3]/cluster\_ctrl](abi-testing.html#abi-sys-kernel-debug-hisi-hpre-bdf-cluster-0-3-cluster-ctrl)
* [/sys/kernel/debug/hisi\_hpre/<bdf>/rdclr\_en](abi-testing.html#abi-sys-kernel-debug-hisi-hpre-bdf-rdclr-en)
* [/sys/kernel/debug/hisi\_hpre/<bdf>/current\_qm](abi-testing.html#abi-sys-kernel-debug-hisi-hpre-bdf-current-qm)
* [/sys/kernel/debug/hisi\_hpre/<bdf>/alg\_qos](abi-testing.html#abi-sys-kernel-debug-hisi-hpre-bdf-alg-qos)
* [/sys/kernel/debug/hisi\_hpre/<bdf>/regs](abi-testing.html#abi-sys-kernel-debug-hisi-hpre-bdf-regs)
* [/sys/kernel/debug/hisi\_hpre/<bdf>/qm/regs](abi-testing.html#abi-sys-kernel-debug-hisi-hpre-bdf-qm-regs)
* [/sys/kernel/debug/hisi\_hpre/<bdf>/dev\_usage](abi-testing.html#abi-sys-kernel-debug-hisi-hpre-bdf-dev-usage)
* [/sys/kernel/debug/hisi\_hpre/<bdf>/qm/current\_q](abi-testing.html#abi-sys-kernel-debug-hisi-hpre-bdf-qm-current-q)
* [/sys/kernel/debug/hisi\_hpre/<bdf>/qm/clear\_enable](abi-testing.html#abi-sys-kernel-debug-hisi-hpre-bdf-qm-clear-enable)
* [/sys/kernel/debug/hisi\_hpre/<bdf>/qm/err\_irq](abi-testing.html#abi-sys-kernel-debug-hisi-hpre-bdf-qm-err-irq)
* [/sys/kernel/debug/hisi\_hpre/<bdf>/qm/aeq\_irq](abi-testing.html#abi-sys-kernel-debug-hisi-hpre-bdf-qm-aeq-irq)
* [/sys/kernel/debug/hisi\_hpre/<bdf>/qm/abnormal\_irq](abi-testing.html#abi-sys-kernel-debug-hisi-hpre-bdf-qm-abnormal-irq)
* [/sys/kernel/debug/hisi\_hpre/<bdf>/qm/create\_qp\_err](abi-testing.html#abi-sys-kernel-debug-hisi-hpre-bdf-qm-create-qp-err)
* [/sys/kernel/debug/hisi\_hpre/<bdf>/qm/mb\_err](abi-testing.html#abi-sys-kernel-debug-hisi-hpre-bdf-qm-mb-err)
* [/sys/kernel/debug/hisi\_hpre/<bdf>/qm/status](abi-testing.html#abi-sys-kernel-debug-hisi-hpre-bdf-qm-status)
* [/sys/kernel/debug/hisi\_hpre/<bdf>/qm/diff\_regs](abi-testing.html#abi-sys-kernel-debug-hisi-hpre-bdf-qm-diff-regs)
* [/sys/kernel/debug/hisi\_hpre/<bdf>/qm/qm\_state](abi-testing.html#abi-sys-kernel-debug-hisi-hpre-bdf-qm-qm-state)
* [/sys/kernel/debug/hisi\_hpre/<bdf>/qm/dev\_timeout](abi-testing.html#abi-sys-kernel-debug-hisi-hpre-bdf-qm-dev-timeout)
* [/sys/kernel/debug/hisi\_hpre/<bdf>/qm/dev\_state](abi-testing.html#abi-sys-kernel-debug-hisi-hpre-bdf-qm-dev-state)
* [/sys/kernel/debug/hisi\_hpre/<bdf>/hpre\_dfx/diff\_regs](abi-testing.html#abi-sys-kernel-debug-hisi-hpre-bdf-hpre-dfx-diff-regs)
* [/sys/kernel/debug/hisi\_hpre/<bdf>/hpre\_dfx/send\_cnt](abi-testing.html#abi-sys-kernel-debug-hisi-hpre-bdf-hpre-dfx-send-cnt)
* [/sys/kernel/debug/hisi\_hpre/<bdf>/hpre\_dfx/recv\_cnt](abi-testing.html#abi-sys-kernel-debug-hisi-hpre-bdf-hpre-dfx-recv-cnt)
* [/sys/kernel/debug/hisi\_hpre/<bdf>/hpre\_dfx/send\_busy\_cnt](abi-testing.html#abi-sys-kernel-debug-hisi-hpre-bdf-hpre-dfx-send-busy-cnt)
* [/sys/kernel/debug/hisi\_hpre/<bdf>/hpre\_dfx/send\_fail\_cnt](abi-testing.html#abi-sys-kernel-debug-hisi-hpre-bdf-hpre-dfx-send-fail-cnt)
* [/sys/kernel/debug/hisi\_hpre/<bdf>/hpre\_dfx/invalid\_req\_cnt](abi-testing.html#abi-sys-kernel-debug-hisi-hpre-bdf-hpre-dfx-invalid-req-cnt)
* [/sys/kernel/debug/hisi\_hpre/<bdf>/hpre\_dfx/overtime\_thrhld](abi-testing.html#abi-sys-kernel-debug-hisi-hpre-bdf-hpre-dfx-overtime-thrhld)
* [/sys/kernel/debug/hisi\_hpre/<bdf>/hpre\_dfx/over\_thrhld\_cnt](abi-testing.html#abi-sys-kernel-debug-hisi-hpre-bdf-hpre-dfx-over-thrhld-cnt)
* [/sys/kernel/debug/hisi\_hpre/<bdf>/cap\_regs](abi-testing.html#abi-sys-kernel-debug-hisi-hpre-bdf-cap-regs)

## ABI file testing/debugfs-hisi-migration

Has the following ABI:

* [/sys/kernel/debug/vfio/<device>/migration/hisi\_acc/dev\_data](abi-testing.html#abi-sys-kernel-debug-vfio-device-migration-hisi-acc-dev-data)
* [/sys/kernel/debug/vfio/<device>/migration/hisi\_acc/migf\_data](abi-testing.html#abi-sys-kernel-debug-vfio-device-migration-hisi-acc-migf-data)
* [/sys/kernel/debug/vfio/<device>/migration/hisi\_acc/cmd\_state](abi-testing.html#abi-sys-kernel-debug-vfio-device-migration-hisi-acc-cmd-state)

## ABI file testing/debugfs-hisi-sec

Has the following ABI:

* [/sys/kernel/debug/hisi\_sec2/<bdf>/clear\_enable](abi-testing.html#abi-sys-kernel-debug-hisi-sec2-bdf-clear-enable)
* [/sys/kernel/debug/hisi\_sec2/<bdf>/current\_qm](abi-testing.html#abi-sys-kernel-debug-hisi-sec2-bdf-current-qm)
* [/sys/kernel/debug/hisi\_sec2/<bdf>/alg\_qos](abi-testing.html#abi-sys-kernel-debug-hisi-sec2-bdf-alg-qos)
* [/sys/kernel/debug/hisi\_sec2/<bdf>/dev\_usage](abi-testing.html#abi-sys-kernel-debug-hisi-sec2-bdf-dev-usage)
* [/sys/kernel/debug/hisi\_sec2/<bdf>/qm/qm\_regs](abi-testing.html#abi-sys-kernel-debug-hisi-sec2-bdf-qm-qm-regs)
* [/sys/kernel/debug/hisi\_sec2/<bdf>/qm/current\_q](abi-testing.html#abi-sys-kernel-debug-hisi-sec2-bdf-qm-current-q)
* [/sys/kernel/debug/hisi\_sec2/<bdf>/qm/clear\_enable](abi-testing.html#abi-sys-kernel-debug-hisi-sec2-bdf-qm-clear-enable)
* [/sys/kernel/debug/hisi\_sec2/<bdf>/qm/err\_irq](abi-testing.html#abi-sys-kernel-debug-hisi-sec2-bdf-qm-err-irq)
* [/sys/kernel/debug/hisi\_sec2/<bdf>/qm/aeq\_irq](abi-testing.html#abi-sys-kernel-debug-hisi-sec2-bdf-qm-aeq-irq)
* [/sys/kernel/debug/hisi\_sec2/<bdf>/qm/abnormal\_irq](abi-testing.html#abi-sys-kernel-debug-hisi-sec2-bdf-qm-abnormal-irq)
* [/sys/kernel/debug/hisi\_sec2/<bdf>/qm/create\_qp\_err](abi-testing.html#abi-sys-kernel-debug-hisi-sec2-bdf-qm-create-qp-err)
* [/sys/kernel/debug/hisi\_sec2/<bdf>/qm/mb\_err](abi-testing.html#abi-sys-kernel-debug-hisi-sec2-bdf-qm-mb-err)
* [/sys/kernel/debug/hisi\_sec2/<bdf>/qm/status](abi-testing.html#abi-sys-kernel-debug-hisi-sec2-bdf-qm-status)
* [/sys/kernel/debug/hisi\_sec2/<bdf>/qm/diff\_regs](abi-testing.html#abi-sys-kernel-debug-hisi-sec2-bdf-qm-diff-regs)
* [/sys/kernel/debug/hisi\_sec2/<bdf>/qm/qm\_state](abi-testing.html#abi-sys-kernel-debug-hisi-sec2-bdf-qm-qm-state)
* [/sys/kernel/debug/hisi\_sec2/<bdf>/qm/dev\_timeout](abi-testing.html#abi-sys-kernel-debug-hisi-sec2-bdf-qm-dev-timeout)
* [/sys/kernel/debug/hisi\_sec2/<bdf>/qm/dev\_state](abi-testing.html#abi-sys-kernel-debug-hisi-sec2-bdf-qm-dev-state)
* [/sys/kernel/debug/hisi\_sec2/<bdf>/sec\_dfx/diff\_regs](abi-testing.html#abi-sys-kernel-debug-hisi-sec2-bdf-sec-dfx-diff-regs)
* [/sys/kernel/debug/hisi\_sec2/<bdf>/sec\_dfx/send\_cnt](abi-testing.html#abi-sys-kernel-debug-hisi-sec2-bdf-sec-dfx-send-cnt)
* [/sys/kernel/debug/hisi\_sec2/<bdf>/sec\_dfx/recv\_cnt](abi-testing.html#abi-sys-kernel-debug-hisi-sec2-bdf-sec-dfx-recv-cnt)
* [/sys/kernel/debug/hisi\_sec2/<bdf>/sec\_dfx/send\_busy\_cnt](abi-testing.html#abi-sys-kernel-debug-hisi-sec2-bdf-sec-dfx-send-busy-cnt)
* [/sys/kernel/debug/hisi\_sec2/<bdf>/sec\_dfx/err\_bd\_cnt](abi-testing.html#abi-sys-kernel-debug-hisi-sec2-bdf-sec-dfx-err-bd-cnt)
* [/sys/kernel/debug/hisi\_sec2/<bdf>/sec\_dfx/invalid\_req\_cnt](abi-testing.html#abi-sys-kernel-debug-hisi-sec2-bdf-sec-dfx-invalid-req-cnt)
* [/sys/kernel/debug/hisi\_sec2/<bdf>/sec\_dfx/done\_flag\_cnt](abi-testing.html#abi-sys-kernel-debug-hisi-sec2-bdf-sec-dfx-done-flag-cnt)
* [/sys/kernel/debug/hisi\_sec2/<bdf>/cap\_regs](abi-testing.html#abi-sys-kernel-debug-hisi-sec2-bdf-cap-regs)

## ABI file testing/debugfs-hisi-zip

Has the following ABI:

* [/sys/kernel/debug/hisi\_zip/<bdf>/comp\_core[01]/regs](abi-testing.html#abi-sys-kernel-debug-hisi-zip-bdf-comp-core-01-regs)
* [/sys/kernel/debug/hisi\_zip/<bdf>/decomp\_core[0-5]/regs](abi-testing.html#abi-sys-kernel-debug-hisi-zip-bdf-decomp-core-0-5-regs)
* [/sys/kernel/debug/hisi\_zip/<bdf>/clear\_enable](abi-testing.html#abi-sys-kernel-debug-hisi-zip-bdf-clear-enable)
* [/sys/kernel/debug/hisi\_zip/<bdf>/current\_qm](abi-testing.html#abi-sys-kernel-debug-hisi-zip-bdf-current-qm)
* [/sys/kernel/debug/hisi\_zip/<bdf>/alg\_qos](abi-testing.html#abi-sys-kernel-debug-hisi-zip-bdf-alg-qos)
* [/sys/kernel/debug/hisi\_zip/<bdf>/dev\_usage](abi-testing.html#abi-sys-kernel-debug-hisi-zip-bdf-dev-usage)
* [/sys/kernel/debug/hisi\_zip/<bdf>/qm/regs](abi-testing.html#abi-sys-kernel-debug-hisi-zip-bdf-qm-regs)
* [/sys/kernel/debug/hisi\_zip/<bdf>/qm/current\_q](abi-testing.html#abi-sys-kernel-debug-hisi-zip-bdf-qm-current-q)
* [/sys/kernel/debug/hisi\_zip/<bdf>/qm/clear\_enable](abi-testing.html#abi-sys-kernel-debug-hisi-zip-bdf-qm-clear-enable)
* [/sys/kernel/debug/hisi\_zip/<bdf>/qm/err\_irq](abi-testing.html#abi-sys-kernel-debug-hisi-zip-bdf-qm-err-irq)
* [/sys/kernel/debug/hisi\_zip/<bdf>/qm/aeq\_irq](abi-testing.html#abi-sys-kernel-debug-hisi-zip-bdf-qm-aeq-irq)
* [/sys/kernel/debug/hisi\_zip/<bdf>/qm/abnormal\_irq](abi-testing.html#abi-sys-kernel-debug-hisi-zip-bdf-qm-abnormal-irq)
* [/sys/kernel/debug/hisi\_zip/<bdf>/qm/create\_qp\_err](abi-testing.html#abi-sys-kernel-debug-hisi-zip-bdf-qm-create-qp-err)
* [/sys/kernel/debug/hisi\_zip/<bdf>/qm/mb\_err](abi-testing.html#abi-sys-kernel-debug-hisi-zip-bdf-qm-mb-err)
* [/sys/kernel/debug/hisi\_zip/<bdf>/qm/status](abi-testing.html#abi-sys-kernel-debug-hisi-zip-bdf-qm-status)
* [/sys/kernel/debug/hisi\_zip/<bdf>/qm/diff\_regs](abi-testing.html#abi-sys-kernel-debug-hisi-zip-bdf-qm-diff-regs)
* [/sys/kernel/debug/hisi\_zip/<bdf>/qm/qm\_state](abi-testing.html#abi-sys-kernel-debug-hisi-zip-bdf-qm-qm-state)
* [/sys/kernel/debug/hisi\_zip/<bdf>/qm/dev\_timeout](abi-testing.html#abi-sys-kernel-debug-hisi-zip-bdf-qm-dev-timeout)
* [/sys/kernel/debug/hisi\_zip/<bdf>/qm/dev\_state](abi-testing.html#abi-sys-kernel-debug-hisi-zip-bdf-qm-dev-state)
* [/sys/kernel/debug/hisi\_zip/<bdf>/zip\_dfx/diff\_regs](abi-testing.html#abi-sys-kernel-debug-hisi-zip-bdf-zip-dfx-diff-regs)
* [/sys/kernel/debug/hisi\_zip/<bdf>/zip\_dfx/send\_cnt](abi-testing.html#abi-sys-kernel-debug-hisi-zip-bdf-zip-dfx-send-cnt)
* [/sys/kernel/debug/hisi\_zip/<bdf>/zip\_dfx/recv\_cnt](abi-testing.html#abi-sys-kernel-debug-hisi-zip-bdf-zip-dfx-recv-cnt)
* [/sys/kernel/debug/hisi\_zip/<bdf>/zip\_dfx/send\_busy\_cnt](abi-testing.html#abi-sys-kernel-debug-hisi-zip-bdf-zip-dfx-send-busy-cnt)
* [/sys/kernel/debug/hisi\_zip/<bdf>/zip\_dfx/err\_bd\_cnt](abi-testing.html#abi-sys-kernel-debug-hisi-zip-bdf-zip-dfx-err-bd-cnt)
* [/sys/kernel/debug/hisi\_zip/<bdf>/cap\_regs](abi-testing.html#abi-sys-kernel-debug-hisi-zip-bdf-cap-regs)

## ABI file testing/debugfs-hyperv

Has the following ABI:

* [/sys/kernel/debug/hyperv/<UUID>/fuzz\_test\_state](abi-testing.html#abi-sys-kernel-debug-hyperv-uuid-fuzz-test-state)
* [/sys/kernel/debug/hyperv/<UUID>/delay/fuzz\_test\_buffer\_interrupt\_delay](abi-testing.html#abi-sys-kernel-debug-hyperv-uuid-delay-fuzz-test-buffer-interrupt-delay)
* [/sys/kernel/debug/hyperv/<UUID>/delay/fuzz\_test\_message\_delay](abi-testing.html#abi-sys-kernel-debug-hyperv-uuid-delay-fuzz-test-message-delay)

## ABI file testing/debugfs-ideapad

Has the following ABI:

* [/sys/kernel/debug/ideapad/cfg](abi-testing.html#abi-sys-kernel-debug-ideapad-cfg)
* [/sys/kernel/debug/ideapad/status](abi-testing.html#abi-sys-kernel-debug-ideapad-status)

## ABI file testing/debugfs-iio-ad9467

Has the following ABI:

* [/sys/kernel/debug/iio/iio:deviceX/calibration\_table\_dump](abi-testing.html#abi-sys-kernel-debug-iio-iio-devicex-calibration-table-dump)
* [/sys/kernel/debug/iio/iio:deviceX/in\_voltage\_test\_mode\_available](abi-testing.html#abi-sys-kernel-debug-iio-iio-devicex-in-voltage-test-mode-available)
* [/sys/kernel/debug/iio/iio:deviceX/in\_voltageY\_test\_mode](abi-testing.html#abi-sys-kernel-debug-iio-iio-devicex-in-voltagey-test-mode)

## ABI file testing/debugfs-iio-backend

Has the following ABI:

* [/sys/kernel/debug/iio/iio:deviceX/backendY/name](abi-testing.html#abi-sys-kernel-debug-iio-iio-devicex-backendy-name)
* [/sys/kernel/debug/iio/iio:deviceX/backendY/direct\_reg\_access](abi-testing.html#abi-sys-kernel-debug-iio-iio-devicex-backendy-direct-reg-access)

## ABI file testing/debugfs-intel-iommu

Has the following ABI:

* [/sys/kernel/debug/iommu/intel/iommu\_regset](abi-testing.html#abi-sys-kernel-debug-iommu-intel-iommu-regset)
* [/sys/kernel/debug/iommu/intel/ir\_translation\_struct](abi-testing.html#abi-sys-kernel-debug-iommu-intel-ir-translation-struct)
* [/sys/kernel/debug/iommu/intel/dmar\_translation\_struct](abi-testing.html#abi-sys-kernel-debug-iommu-intel-dmar-translation-struct)
* [/sys/kernel/debug/iommu/intel/invalidation\_queue](abi-testing.html#abi-sys-kernel-debug-iommu-intel-invalidation-queue)
* [/sys/kernel/debug/iommu/intel/dmar\_perf\_latency](abi-testing.html#abi-sys-kernel-debug-iommu-intel-dmar-perf-latency)
* [/sys/kernel/debug/iommu/intel/<bdf>/domain\_translation\_struct](abi-testing.html#abi-sys-kernel-debug-iommu-intel-bdf-domain-translation-struct)

## ABI file testing/debugfs-moxtet

Has the following ABI:

* [/sys/kernel/debug/moxtet/input](abi-testing.html#abi-sys-kernel-debug-moxtet-input)
* [/sys/kernel/debug/moxtet/output](abi-testing.html#abi-sys-kernel-debug-moxtet-output)

## ABI file testing/debugfs-msi-wmi-platform

Has the following ABI:

* [/sys/kernel/debug/msi-wmi-platform-<wmi\_device\_name>/\*](abi-testing.html#abi-sys-kernel-debug-msi-wmi-platform-wmi-device-name)

## ABI file testing/debugfs-olpc

Has the following ABI:

* [/sys/kernel/debug/olpc-ec/cmd](abi-testing.html#abi-sys-kernel-debug-olpc-ec-cmd)

## ABI file testing/debugfs-pcie-ptm

Has the following ABI:

* [/sys/kernel/debug/pcie\_ptm\_\*/local\_clock](abi-testing.html#abi-sys-kernel-debug-pcie-ptm-local-clock)
* [/sys/kernel/debug/pcie\_ptm\_\*/master\_clock](abi-testing.html#abi-sys-kernel-debug-pcie-ptm-master-clock)
* [/sys/kernel/debug/pcie\_ptm\_\*/t1](abi-testing.html#abi-sys-kernel-debug-pcie-ptm-t1)
* [/sys/kernel/debug/pcie\_ptm\_\*/t2](abi-testing.html#abi-sys-kernel-debug-pcie-ptm-t2)
* [/sys/kernel/debug/pcie\_ptm\_\*/t3](abi-testing.html#abi-sys-kernel-debug-pcie-ptm-t3)
* [/sys/kernel/debug/pcie\_ptm\_\*/t4](abi-testing.html#abi-sys-kernel-debug-pcie-ptm-t4)
* [/sys/kernel/debug/pcie\_ptm\_\*/context\_update](abi-testing.html#abi-sys-kernel-debug-pcie-ptm-context-update)
* [/sys/kernel/debug/pcie\_ptm\_\*/context\_valid](abi-testing.html#abi-sys-kernel-debug-pcie-ptm-context-valid)

## ABI file testing/debugfs-pfo-nx-crypto

Has the following ABI:

* [/sys/kernel/debug/nx-crypto/\*](abi-testing.html#abi-sys-kernel-debug-nx-crypto)

## ABI file testing/debugfs-scmi

Has the following ABI:

* [/sys/kernel/debug/scmi/<n>/instance\_name](abi-testing.html#abi-sys-kernel-debug-scmi-n-instance-name)
* [/sys/kernel/debug/scmi/<n>/atomic\_threshold\_us](abi-testing.html#abi-sys-kernel-debug-scmi-n-atomic-threshold-us)
* [/sys/kernel/debug/scmi/<n>/transport/type](abi-testing.html#abi-sys-kernel-debug-scmi-n-transport-type)
* [/sys/kernel/debug/scmi/<n>/transport/is\_atomic](abi-testing.html#abi-sys-kernel-debug-scmi-n-transport-is-atomic)
* [/sys/kernel/debug/scmi/<n>/transport/max\_rx\_timeout\_ms](abi-testing.html#abi-sys-kernel-debug-scmi-n-transport-max-rx-timeout-ms)
* [/sys/kernel/debug/scmi/<n>/transport/max\_msg\_size](abi-testing.html#abi-sys-kernel-debug-scmi-n-transport-max-msg-size)
* [/sys/kernel/debug/scmi/<n>/transport/tx\_max\_msg](abi-testing.html#abi-sys-kernel-debug-scmi-n-transport-tx-max-msg)
* [/sys/kernel/debug/scmi/<n>/transport/rx\_max\_msg](abi-testing.html#abi-sys-kernel-debug-scmi-n-transport-rx-max-msg)

## ABI file testing/debugfs-scmi-raw

Has the following ABI:

* [/sys/kernel/debug/scmi/<n>/raw/message](abi-testing.html#abi-sys-kernel-debug-scmi-n-raw-message)
* [/sys/kernel/debug/scmi/<n>/raw/message\_async](abi-testing.html#abi-sys-kernel-debug-scmi-n-raw-message-async)
* [/sys/kernel/debug/scmi/<n>/raw/message\_poll](abi-testing.html#abi-sys-kernel-debug-scmi-n-raw-message-poll)
* [/sys/kernel/debug/scmi/<n>/raw/message\_poll\_async](abi-testing.html#abi-sys-kernel-debug-scmi-n-raw-message-poll-async)
* [/sys/kernel/debug/scmi/<n>/raw/errors](abi-testing.html#abi-sys-kernel-debug-scmi-n-raw-errors)
* [/sys/kernel/debug/scmi/<n>/raw/notification](abi-testing.html#abi-sys-kernel-debug-scmi-n-raw-notification)
* [/sys/kernel/debug/scmi/<n>/raw/reset](abi-testing.html#abi-sys-kernel-debug-scmi-n-raw-reset)
* [/sys/kernel/debug/scmi/<n>/raw/channels/<m>/message](abi-testing.html#abi-sys-kernel-debug-scmi-n-raw-channels-m-message)
* [/sys/kernel/debug/scmi/<n>/raw/channels/<m>/message\_async](abi-testing.html#abi-sys-kernel-debug-scmi-n-raw-channels-m-message-async)
* [/sys/kernel/debug/scmi/<n>/raw/channels/<m>/message\_poll](abi-testing.html#abi-sys-kernel-debug-scmi-n-raw-channels-m-message-poll)
* [/sys/kernel/debug/scmi/<n>/raw/channels/<m>/message\_poll\_async](abi-testing.html#abi-sys-kernel-debug-scmi-n-raw-channels-m-message-poll-async)

## ABI file testing/debugfs-tpmi

Has the following ABI:

* [/sys/kernel/debug/tpmi-<n>/pfs\_dump](abi-testing.html#abi-sys-kernel-debug-tpmi-n-pfs-dump)
* [/sys/kernel/debug/tpmi-<n>/tpmi-id-<n>/mem\_dump](abi-testing.html#abi-sys-kernel-debug-tpmi-n-tpmi-id-n-mem-dump)
* [/sys/kernel/debug/tpmi-<n>/tpmi-id-<n>/mem\_write](abi-testing.html#abi-sys-kernel-debug-tpmi-n-tpmi-id-n-mem-write)
* [/sys/kernel/debug/tpmi-<n>/plr/domain<n>/status](abi-testing.html#abi-sys-kernel-debug-tpmi-n-plr-domain-n-status)

## ABI file testing/debugfs-vfio

Has the following ABI:

* [/sys/kernel/debug/vfio](abi-testing.html#abi-sys-kernel-debug-vfio)
* [/sys/kernel/debug/vfio/<device>/migration](abi-testing.html#abi-sys-kernel-debug-vfio-device-migration)
* [/sys/kernel/debug/vfio/<device>/migration/state](abi-testing.html#abi-sys-kernel-debug-vfio-device-migration-state)
* [/sys/kernel/debug/vfio/<device>/migration/features](abi-testing.html#abi-sys-kernel-debug-vfio-device-migration-features)

## ABI file testing/debugfs-wilco-ec

Has the following ABI:

* [/sys/kernel/debug/wilco\_ec/h1\_gpio](abi-testing.html#abi-sys-kernel-debug-wilco-ec-h1-gpio)
* [/sys/kernel/debug/wilco\_ec/raw](abi-testing.html#abi-sys-kernel-debug-wilco-ec-raw)

## ABI file testing/dell-smbios-wmi

Has the following ABI:

* [/dev/wmi/dell-smbios](abi-testing.html#abi-dev-wmi-dell-smbios)

## ABI file testing/dev-kmsg

Has the following ABI:

* [/dev/kmsg](abi-testing.html#abi-dev-kmsg)

## ABI file testing/devlink-resource-mlxsw

Has the following ABI:

* [/kvd/](abi-testing.html#abi-kvd)
* [/kvd/linear](abi-testing.html#abi-kvd-linear)
* [/kvd/hash\_single](abi-testing.html#abi-kvd-hash-single)
* [/kvd/hash\_double](abi-testing.html#abi-kvd-hash-double)

## ABI file testing/evm

Has the following ABI:

* [/sys/kernel/security/evm](abi-testing.html#abi-sys-kernel-security-evm)
* [/sys/kernel/security/\*/evm](abi-testing.html#abi-sys-kernel-security-evm)
* [/sys/kernel/security/\*/evm/evm\_xattrs](abi-testing.html#abi-sys-kernel-security-evm-evm-xattrs)

## ABI file testing/gpio-cdev

Has the following ABI:

* [/dev/gpiochip[0-9]+](abi-testing.html#abi-dev-gpiochip-0-9)

## ABI file testing/ima\_policy

Has the following ABI:

* [/sys/kernel/security/\*/ima/policy](abi-testing.html#abi-sys-kernel-security-ima-policy)

## ABI file testing/ppc-memtrace

Has the following ABI:

* [/sys/kernel/debug/powerpc/memtrace](abi-testing.html#abi-sys-kernel-debug-powerpc-memtrace)
* [/sys/kernel/debug/powerpc/memtrace/enable](abi-testing.html#abi-sys-kernel-debug-powerpc-memtrace-enable)
* [/sys/kernel/debug/powerpc/memtrace/<node-id>](abi-testing.html#abi-sys-kernel-debug-powerpc-memtrace-node-id)
* [/sys/kernel/debug/powerpc/memtrace/<node-id>/size](abi-testing.html#abi-sys-kernel-debug-powerpc-memtrace-node-id-size)
* [/sys/kernel/debug/powerpc/memtrace/<node-id>/start](abi-testing.html#abi-sys-kernel-debug-powerpc-memtrace-node-id-start)
* [/sys/kernel/debug/powerpc/memtrace/<node-id>/trace](abi-testing.html#abi-sys-kernel-debug-powerpc-memtrace-node-id-trace)

## ABI file testing/procfs-attr-current

Has the following ABI:

* [/proc/\*/attr/current](abi-testing.html#abi-proc-attr-current)

## ABI file testing/procfs-attr-exec

Has the following ABI:

* [/proc/\*/attr/exec](abi-testing.html#abi-proc-attr-exec)

## ABI file testing/procfs-attr-prev

Has the following ABI:

* [/proc/\*/attr/prev](abi-testing.html#abi-proc-attr-prev)

## ABI file testing/procfs-diskstats

Has the following ABI:

* [/proc/diskstats](abi-testing.html#abi-proc-diskstats)

## ABI file testing/procfs-smaps\_rollup

Has the following ABI:

* [/proc/pid/smaps\_rollup](abi-testing.html#abi-proc-pid-smaps-rollup)

## ABI file testing/pstore

Has the following ABI:

* [/sys/fs/pstore/...](abi-testing.html#abi-sys-fs-pstore)
* [/dev/pstore/...](abi-testing.html#abi-sys-fs-pstore)

## ABI file testing/rtc-cdev

Has the following ABI:

* [/dev/rtcX](abi-testing.html#abi-dev-rtcx)

## ABI file testing/securityfs-secrets-coco

Has the following ABI:

* [security/secrets/coco](abi-testing.html#abi-security-secrets-coco)

## ABI file testing/sysfs-amd-pmc

Has the following ABI:

* [/sys/bus/platform/drivers/amd\_pmc/\*/smu\_fw\_version](abi-testing.html#abi-sys-bus-platform-drivers-amd-pmc-smu-fw-version)
* [/sys/bus/platform/drivers/amd\_pmc/\*/smu\_program](abi-testing.html#abi-sys-bus-platform-drivers-amd-pmc-smu-program)

## ABI file testing/sysfs-amd-pmf

Has the following ABI:

* [/sys/devices/platform/\*/cnqf\_enable](abi-testing.html#abi-sys-devices-platform-cnqf-enable)

## ABI file testing/sysfs-ata

Has the following ABI:

* [/sys/class/ata\_\*](abi-testing.html#abi-sys-class-ata)
* [/sys/class/ata\_port/ataX/nr\_pmp\_links](abi-testing.html#abi-sys-class-ata-port-atax-nr-pmp-links)
* [/sys/class/ata\_port/ataX/idle\_irq](abi-testing.html#abi-sys-class-ata-port-atax-nr-pmp-links)
* [/sys/class/ata\_port/ataX/port\_no](abi-testing.html#abi-sys-class-ata-port-atax-port-no)
* [/sys/class/ata\_link/linkX[.Y]/hw\_sata\_spd\_limit](abi-testing.html#abi-sys-class-ata-link-linkx-y-hw-sata-spd-limit)
* [/sys/class/ata\_link/linkX[.Y]/sata\_spd\_limit](abi-testing.html#abi-sys-class-ata-link-linkx-y-hw-sata-spd-limit)
* [/sys/class/ata\_link/linkX[.Y]/sata\_spd](abi-testing.html#abi-sys-class-ata-link-linkx-y-hw-sata-spd-limit)
* [/sys/class/ata\_device/devX[.Y].Z/spdn\_cnt](abi-testing.html#abi-sys-class-ata-device-devx-y-z-spdn-cnt)
* [/sys/class/ata\_device/devX[.Y].Z/gscr](abi-testing.html#abi-sys-class-ata-device-devx-y-z-spdn-cnt)
* [/sys/class/ata\_device/devX[.Y].Z/ering](abi-testing.html#abi-sys-class-ata-device-devx-y-z-spdn-cnt)
* [/sys/class/ata\_device/devX[.Y].Z/id](abi-testing.html#abi-sys-class-ata-device-devx-y-z-spdn-cnt)
* [/sys/class/ata\_device/devX[.Y].Z/pio\_mode](abi-testing.html#abi-sys-class-ata-device-devx-y-z-spdn-cnt)
* [/sys/class/ata\_device/devX[.Y].Z/xfer\_mode](abi-testing.html#abi-sys-class-ata-device-devx-y-z-spdn-cnt)
* [/sys/class/ata\_device/devX[.Y].Z/dma\_mode](abi-testing.html#abi-sys-class-ata-device-devx-y-z-spdn-cnt)
* [/sys/class/ata\_device/devX[.Y].Z/class](abi-testing.html#abi-sys-class-ata-device-devx-y-z-spdn-cnt)
* [/sys/class/ata\_device/devX[.Y].Z/trim](abi-testing.html#abi-sys-class-ata-device-devx-y-z-trim)

## ABI file testing/sysfs-auxdisplay-linedisp

Has the following ABI:

* [/sys/.../message](abi-testing.html#abi-sys-message)
* [/sys/.../num\_chars](abi-testing.html#abi-sys-num-chars)
* [/sys/.../scroll\_step\_ms](abi-testing.html#abi-sys-scroll-step-ms)
* [/sys/.../map\_seg7](abi-testing.html#abi-sys-map-seg7)
* [/sys/.../map\_seg14](abi-testing.html#abi-sys-map-seg14)

## ABI file testing/sysfs-block-aoe

Has the following ABI:

* [/sys/block/etherd\*/mac](abi-testing.html#abi-sys-block-etherd-mac)
* [/sys/block/etherd\*/netif](abi-testing.html#abi-sys-block-etherd-netif)
* [/sys/block/etherd\*/state](abi-testing.html#abi-sys-block-etherd-state)
* [/sys/block/etherd\*/firmware-version](abi-testing.html#abi-sys-block-etherd-firmware-version)
* [/sys/block/etherd\*/payload](abi-testing.html#abi-sys-block-etherd-payload)

## ABI file testing/sysfs-block-bcache

Has the following ABI:

* [/sys/block/<disk>/bcache/unregister](abi-testing.html#abi-sys-block-disk-bcache-unregister)
* [/sys/block/<disk>/bcache/clear\_stats](abi-testing.html#abi-sys-block-disk-bcache-clear-stats)
* [/sys/block/<disk>/bcache/cache](abi-testing.html#abi-sys-block-disk-bcache-cache)
* [/sys/block/<disk>/bcache/cache\_hits](abi-testing.html#abi-sys-block-disk-bcache-cache-hits)
* [/sys/block/<disk>/bcache/cache\_misses](abi-testing.html#abi-sys-block-disk-bcache-cache-misses)
* [/sys/block/<disk>/bcache/cache\_hit\_ratio](abi-testing.html#abi-sys-block-disk-bcache-cache-hit-ratio)
* [/sys/block/<disk>/bcache/sequential\_cutoff](abi-testing.html#abi-sys-block-disk-bcache-sequential-cutoff)
* [/sys/block/<disk>/bcache/bypassed](abi-testing.html#abi-sys-block-disk-bcache-bypassed)
* [/sys/block/<disk>/bcache/writeback](abi-testing.html#abi-sys-block-disk-bcache-writeback)
* [/sys/block/<disk>/bcache/writeback\_running](abi-testing.html#abi-sys-block-disk-bcache-writeback-running)
* [/sys/block/<disk>/bcache/writeback\_delay](abi-testing.html#abi-sys-block-disk-bcache-writeback-delay)
* [/sys/block/<disk>/bcache/writeback\_percent](abi-testing.html#abi-sys-block-disk-bcache-writeback-percent)
* [/sys/block/<disk>/bcache/synchronous](abi-testing.html#abi-sys-block-disk-bcache-synchronous)
* [/sys/block/<disk>/bcache/bucket\_size](abi-testing.html#abi-sys-block-disk-bcache-bucket-size)
* [/sys/block/<disk>/bcache/nbuckets](abi-testing.html#abi-sys-block-disk-bcache-nbuckets)
* [/sys/block/<disk>/bcache/tree\_depth](abi-testing.html#abi-sys-block-disk-bcache-tree-depth)
* [/sys/block/<disk>/bcache/btree\_cache\_size](abi-testing.html#abi-sys-block-disk-bcache-btree-cache-size)
* [/sys/block/<disk>/bcache/written](abi-testing.html#abi-sys-block-disk-bcache-written)
* [/sys/block/<disk>/bcache/btree\_written](abi-testing.html#abi-sys-block-disk-bcache-btree-written)

## ABI file testing/sysfs-block-device

Has the following ABI:

* [/sys/block/\*/device/sw\_activity](abi-testing.html#abi-sys-block-device-sw-activity)
* [/sys/block/\*/device/unload\_heads](abi-testing.html#abi-sys-block-device-unload-heads)
* [/sys/block/\*/device/ncq\_prio\_enable](abi-testing.html#abi-sys-block-device-ncq-prio-enable)
* [/sys/block/\*/device/sas\_ncq\_prio\_enable](abi-testing.html#abi-sys-block-device-sas-ncq-prio-enable)
* [/sys/block/\*/device/ncq\_prio\_supported](abi-testing.html#abi-sys-block-device-ncq-prio-supported)
* [/sys/block/\*/device/sas\_ncq\_prio\_supported](abi-testing.html#abi-sys-block-device-sas-ncq-prio-supported)
* [/sys/block/\*/device/cdl\_supported](abi-testing.html#abi-sys-block-device-cdl-supported)
* [/sys/block/\*/device/cdl\_enable](abi-testing.html#abi-sys-block-device-cdl-enable)

## ABI file testing/sysfs-block-dm

Has the following ABI:

* [/sys/block/dm-<num>/dm/name](abi-testing.html#abi-sys-block-dm-num-dm-name)
* [/sys/block/dm-<num>/dm/uuid](abi-testing.html#abi-sys-block-dm-num-dm-uuid)
* [/sys/block/dm-<num>/dm/suspended](abi-testing.html#abi-sys-block-dm-num-dm-suspended)
* [/sys/block/dm-<num>/dm/rq\_based\_seq\_io\_merge\_deadline](abi-testing.html#abi-sys-block-dm-num-dm-rq-based-seq-io-merge-deadline)
* [/sys/block/dm-<num>/dm/use\_blk\_mq](abi-testing.html#abi-sys-block-dm-num-dm-use-blk-mq)

## ABI file testing/sysfs-block-loop

Has the following ABI:

* [/sys/block/loopX/loop/autoclear](abi-testing.html#abi-sys-block-loopx-loop-autoclear)
* [/sys/block/loopX/loop/backing\_file](abi-testing.html#abi-sys-block-loopx-loop-backing-file)
* [/sys/block/loopX/loop/offset](abi-testing.html#abi-sys-block-loopx-loop-offset)
* [/sys/block/loopX/loop/sizelimit](abi-testing.html#abi-sys-block-loopx-loop-sizelimit)
* [/sys/block/loopX/loop/partscan](abi-testing.html#abi-sys-block-loopx-loop-partscan)
* [/sys/block/loopX/loop/dio](abi-testing.html#abi-sys-block-loopx-loop-dio)

## ABI file testing/sysfs-block-rnbd

Has the following ABI:

* [/sys/block/rnbd<N>/rnbd/unmap\_device](abi-testing.html#abi-sys-block-rnbd-n-rnbd-unmap-device)
* [/sys/block/rnbd<N>/rnbd/state](abi-testing.html#abi-sys-block-rnbd-n-rnbd-state)
* [/sys/block/rnbd<N>/rnbd/session](abi-testing.html#abi-sys-block-rnbd-n-rnbd-session)
* [/sys/block/rnbd<N>/rnbd/mapping\_path](abi-testing.html#abi-sys-block-rnbd-n-rnbd-mapping-path)
* [/sys/block/rnbd<N>/rnbd/access\_mode](abi-testing.html#abi-sys-block-rnbd-n-rnbd-access-mode)
* [/sys/block/rnbd<N>/rnbd/resize](abi-testing.html#abi-sys-block-rnbd-n-rnbd-resize)
* [/sys/block/rnbd<N>/rnbd/remap\_device](abi-testing.html#abi-sys-block-rnbd-n-rnbd-remap-device)
* [/sys/block/rnbd<N>/rnbd/nr\_poll\_queues](abi-testing.html#abi-sys-block-rnbd-n-rnbd-nr-poll-queues)

## ABI file testing/sysfs-block-rssd

Has the following ABI:

* [/sys/block/rssd\*/status](abi-testing.html#abi-sys-block-rssd-status)

## ABI file testing/sysfs-block-zram

Has the following ABI:

* [/sys/block/zram<id>/disksize](abi-testing.html#abi-sys-block-zram-id-disksize)
* [/sys/block/zram<id>/initstate](abi-testing.html#abi-sys-block-zram-id-initstate)
* [/sys/block/zram<id>/reset](abi-testing.html#abi-sys-block-zram-id-reset)
* [/sys/block/zram<id>/comp\_algorithm](abi-testing.html#abi-sys-block-zram-id-comp-algorithm)
* [/sys/block/zram<id>/mem\_used\_max](abi-testing.html#abi-sys-block-zram-id-mem-used-max)
* [/sys/block/zram<id>/mem\_limit](abi-testing.html#abi-sys-block-zram-id-mem-limit)
* [/sys/block/zram<id>/compact](abi-testing.html#abi-sys-block-zram-id-compact)
* [/sys/block/zram<id>/io\_stat](abi-testing.html#abi-sys-block-zram-id-io-stat)
* [/sys/block/zram<id>/mm\_stat](abi-testing.html#abi-sys-block-zram-id-mm-stat)
* [/sys/block/zram<id>/debug\_stat](abi-testing.html#abi-sys-block-zram-id-debug-stat)
* [/sys/block/zram<id>/backing\_dev](abi-testing.html#abi-sys-block-zram-id-backing-dev)
* [/sys/block/zram<id>/idle](abi-testing.html#abi-sys-block-zram-id-idle)
* [/sys/block/zram<id>/writeback](abi-testing.html#abi-sys-block-zram-id-writeback)
* [/sys/block/zram<id>/bd\_stat](abi-testing.html#abi-sys-block-zram-id-bd-stat)
* [/sys/block/zram<id>/writeback\_limit\_enable](abi-testing.html#abi-sys-block-zram-id-writeback-limit-enable)
* [/sys/block/zram<id>/writeback\_limit](abi-testing.html#abi-sys-block-zram-id-writeback-limit)
* [/sys/block/zram<id>/recomp\_algorithm](abi-testing.html#abi-sys-block-zram-id-recomp-algorithm)
* [/sys/block/zram<id>/recompress](abi-testing.html#abi-sys-block-zram-id-recompress)
* [/sys/block/zram<id>/algorithm\_params](abi-testing.html#abi-sys-block-zram-id-algorithm-params)
* [/sys/block/zram<id>/compressed\_writeback](abi-testing.html#abi-sys-block-zram-id-compressed-writeback)
* [/sys/block/zram<id>/writeback\_batch\_size](abi-testing.html#abi-sys-block-zram-id-writeback-batch-size)

## ABI file testing/sysfs-bus-acpi

Has the following ABI:

* [/sys/bus/acpi/devices/.../path](abi-testing.html#abi-sys-bus-acpi-devices-path)
* [/sys/bus/acpi/devices/.../modalias](abi-testing.html#abi-sys-bus-acpi-devices-modalias)
* [/sys/bus/acpi/devices/.../hid](abi-testing.html#abi-sys-bus-acpi-devices-hid)
* [/sys/bus/acpi/devices/.../description](abi-testing.html#abi-sys-bus-acpi-devices-description)
* [/sys/bus/acpi/devices/.../adr](abi-testing.html#abi-sys-bus-acpi-devices-adr)
* [/sys/bus/acpi/devices/.../uid](abi-testing.html#abi-sys-bus-acpi-devices-uid)
* [/sys/bus/acpi/devices/.../eject](abi-testing.html#abi-sys-bus-acpi-devices-eject)
* [/sys/bus/acpi/devices/.../status](abi-testing.html#abi-sys-bus-acpi-devices-status)
* [/sys/bus/acpi/devices/.../hrv](abi-testing.html#abi-sys-bus-acpi-devices-hrv)

## ABI file testing/sysfs-bus-amba

Has the following ABI:

* [/sys/bus/amba/devices/.../driver\_override](abi-testing.html#abi-sys-bus-amba-devices-driver-override)

## ABI file testing/sysfs-bus-auxiliary

Has the following ABI:

* [/sys/bus/auxiliary/devices/.../irqs/](abi-testing.html#abi-sys-bus-auxiliary-devices-irqs)

## ABI file testing/sysfs-bus-bcma

Has the following ABI:

* [/sys/bus/bcma/devices/.../manuf](abi-testing.html#abi-sys-bus-bcma-devices-manuf)
* [/sys/bus/bcma/devices/.../id](abi-testing.html#abi-sys-bus-bcma-devices-id)
* [/sys/bus/bcma/devices/.../rev](abi-testing.html#abi-sys-bus-bcma-devices-rev)
* [/sys/bus/bcma/devices/.../class](abi-testing.html#abi-sys-bus-bcma-devices-class)

## ABI file testing/sysfs-bus-cdx

Has the following ABI:

* [/sys/bus/cdx/rescan](abi-testing.html#abi-sys-bus-cdx-rescan)
* [/sys/bus/cdx/devices/.../vendor](abi-testing.html#abi-sys-bus-cdx-devices-vendor)
* [/sys/bus/cdx/devices/.../device](abi-testing.html#abi-sys-bus-cdx-devices-device)
* [/sys/bus/cdx/devices/.../subsystem\_vendor](abi-testing.html#abi-sys-bus-cdx-devices-subsystem-vendor)
* [/sys/bus/cdx/devices/.../subsystem\_device](abi-testing.html#abi-sys-bus-cdx-devices-subsystem-device)
* [/sys/bus/cdx/devices/.../class](abi-testing.html#abi-sys-bus-cdx-devices-class)
* [/sys/bus/cdx/devices/.../revision](abi-testing.html#abi-sys-bus-cdx-devices-revision)
* [/sys/bus/cdx/devices/.../enable](abi-testing.html#abi-sys-bus-cdx-devices-enable)
* [/sys/bus/cdx/devices/.../reset](abi-testing.html#abi-sys-bus-cdx-devices-reset)
* [/sys/bus/cdx/devices/.../remove](abi-testing.html#abi-sys-bus-cdx-devices-remove)
* [/sys/bus/cdx/devices/.../resource<N>](abi-testing.html#abi-sys-bus-cdx-devices-resource-n)
* [/sys/bus/cdx/devices/.../modalias](abi-testing.html#abi-sys-bus-cdx-devices-modalias)

## ABI file testing/sysfs-bus-coreboot

Has the following ABI:

* [/sys/bus/coreboot](abi-testing.html#abi-sys-bus-coreboot)
* [/sys/bus/coreboot/devices/cbmem-<id>](abi-testing.html#abi-sys-bus-coreboot-devices-cbmem-id)
* [/sys/bus/coreboot/devices/cbmem-<id>/address](abi-testing.html#abi-sys-bus-coreboot-devices-cbmem-id-address)
* [/sys/bus/coreboot/devices/cbmem-<id>/size](abi-testing.html#abi-sys-bus-coreboot-devices-cbmem-id-size)
* [/sys/bus/coreboot/devices/cbmem-<id>/mem](abi-testing.html#abi-sys-bus-coreboot-devices-cbmem-id-mem)

## ABI file testing/sysfs-bus-coresight-devices-cti

Has the following ABI:

* [/sys/bus/coresight/devices/<cti-name>/enable](abi-testing.html#abi-sys-bus-coresight-devices-cti-name-enable)
* [/sys/bus/coresight/devices/<cti-name>/powered](abi-testing.html#abi-sys-bus-coresight-devices-cti-name-powered)
* [/sys/bus/coresight/devices/<cti-name>/ctmid](abi-testing.html#abi-sys-bus-coresight-devices-cti-name-ctmid)
* [/sys/bus/coresight/devices/<cti-name>/nr\_trigger\_cons](abi-testing.html#abi-sys-bus-coresight-devices-cti-name-nr-trigger-cons)
* [/sys/bus/coresight/devices/<cti-name>/triggers<N>/name](abi-testing.html#abi-sys-bus-coresight-devices-cti-name-triggers-n-name)
* [/sys/bus/coresight/devices/<cti-name>/triggers<N>/in\_signals](abi-testing.html#abi-sys-bus-coresight-devices-cti-name-triggers-n-in-signals)
* [/sys/bus/coresight/devices/<cti-name>/triggers<N>/in\_types](abi-testing.html#abi-sys-bus-coresight-devices-cti-name-triggers-n-in-types)
* [/sys/bus/coresight/devices/<cti-name>/triggers<N>/out\_signals](abi-testing.html#abi-sys-bus-coresight-devices-cti-name-triggers-n-out-signals)
* [/sys/bus/coresight/devices/<cti-name>/triggers<N>/out\_types](abi-testing.html#abi-sys-bus-coresight-devices-cti-name-triggers-n-out-types)
* [/sys/bus/coresight/devices/<cti-name>/regs/inout\_sel](abi-testing.html#abi-sys-bus-coresight-devices-cti-name-regs-inout-sel)
* [/sys/bus/coresight/devices/<cti-name>/regs/inen](abi-testing.html#abi-sys-bus-coresight-devices-cti-name-regs-inen)
* [/sys/bus/coresight/devices/<cti-name>/regs/outen](abi-testing.html#abi-sys-bus-coresight-devices-cti-name-regs-outen)
* [/sys/bus/coresight/devices/<cti-name>/regs/gate](abi-testing.html#abi-sys-bus-coresight-devices-cti-name-regs-gate)
* [/sys/bus/coresight/devices/<cti-name>/regs/asicctl](abi-testing.html#abi-sys-bus-coresight-devices-cti-name-regs-asicctl)
* [/sys/bus/coresight/devices/<cti-name>/regs/intack](abi-testing.html#abi-sys-bus-coresight-devices-cti-name-regs-intack)
* [/sys/bus/coresight/devices/<cti-name>/regs/appset](abi-testing.html#abi-sys-bus-coresight-devices-cti-name-regs-appset)
* [/sys/bus/coresight/devices/<cti-name>/regs/appclear](abi-testing.html#abi-sys-bus-coresight-devices-cti-name-regs-appclear)
* [/sys/bus/coresight/devices/<cti-name>/regs/apppulse](abi-testing.html#abi-sys-bus-coresight-devices-cti-name-regs-apppulse)
* [/sys/bus/coresight/devices/<cti-name>/regs/chinstatus](abi-testing.html#abi-sys-bus-coresight-devices-cti-name-regs-chinstatus)
* [/sys/bus/coresight/devices/<cti-name>/regs/choutstatus](abi-testing.html#abi-sys-bus-coresight-devices-cti-name-regs-choutstatus)
* [/sys/bus/coresight/devices/<cti-name>/regs/triginstatus](abi-testing.html#abi-sys-bus-coresight-devices-cti-name-regs-triginstatus)
* [/sys/bus/coresight/devices/<cti-name>/regs/trigoutstatus](abi-testing.html#abi-sys-bus-coresight-devices-cti-name-regs-trigoutstatus)
* [/sys/bus/coresight/devices/<cti-name>/channels/trigin\_attach](abi-testing.html#abi-sys-bus-coresight-devices-cti-name-channels-trigin-attach)
* [/sys/bus/coresight/devices/<cti-name>/channels/trigin\_detach](abi-testing.html#abi-sys-bus-coresight-devices-cti-name-channels-trigin-detach)
* [/sys/bus/coresight/devices/<cti-name>/channels/trigout\_attach](abi-testing.html#abi-sys-bus-coresight-devices-cti-name-channels-trigout-attach)
* [/sys/bus/coresight/devices/<cti-name>/channels/trigout\_detach](abi-testing.html#abi-sys-bus-coresight-devices-cti-name-channels-trigout-detach)
* [/sys/bus/coresight/devices/<cti-name>/channels/chan\_gate\_enable](abi-testing.html#abi-sys-bus-coresight-devices-cti-name-channels-chan-gate-enable)
* [/sys/bus/coresight/devices/<cti-name>/channels/chan\_gate\_disable](abi-testing.html#abi-sys-bus-coresight-devices-cti-name-channels-chan-gate-disable)
* [/sys/bus/coresight/devices/<cti-name>/channels/chan\_set](abi-testing.html#abi-sys-bus-coresight-devices-cti-name-channels-chan-set)
* [/sys/bus/coresight/devices/<cti-name>/channels/chan\_clear](abi-testing.html#abi-sys-bus-coresight-devices-cti-name-channels-chan-clear)
* [/sys/bus/coresight/devices/<cti-name>/channels/chan\_pulse](abi-testing.html#abi-sys-bus-coresight-devices-cti-name-channels-chan-pulse)
* [/sys/bus/coresight/devices/<cti-name>/channels/trigout\_filtered](abi-testing.html#abi-sys-bus-coresight-devices-cti-name-channels-trigout-filtered)
* [/sys/bus/coresight/devices/<cti-name>/channels/trig\_filter\_enable](abi-testing.html#abi-sys-bus-coresight-devices-cti-name-channels-trig-filter-enable)
* [/sys/bus/coresight/devices/<cti-name>/channels/chan\_inuse](abi-testing.html#abi-sys-bus-coresight-devices-cti-name-channels-chan-inuse)
* [/sys/bus/coresight/devices/<cti-name>/channels/chan\_free](abi-testing.html#abi-sys-bus-coresight-devices-cti-name-channels-chan-free)
* [/sys/bus/coresight/devices/<cti-name>/channels/chan\_xtrigs\_sel](abi-testing.html#abi-sys-bus-coresight-devices-cti-name-channels-chan-xtrigs-sel)
* [/sys/bus/coresight/devices/<cti-name>/channels/chan\_xtrigs\_in](abi-testing.html#abi-sys-bus-coresight-devices-cti-name-channels-chan-xtrigs-in)
* [/sys/bus/coresight/devices/<cti-name>/channels/chan\_xtrigs\_out](abi-testing.html#abi-sys-bus-coresight-devices-cti-name-channels-chan-xtrigs-out)
* [/sys/bus/coresight/devices/<cti-name>/channels/chan\_xtrigs\_reset](abi-testing.html#abi-sys-bus-coresight-devices-cti-name-channels-chan-xtrigs-reset)
* [/sys/bus/coresight/devices/<cti-name>/label](abi-testing.html#abi-sys-bus-coresight-devices-cti-name-label)

## ABI file testing/sysfs-bus-coresight-devices-dummy-source

Has the following ABI:

* [/sys/bus/coresight/devices/dummy\_source<N>/enable\_source](abi-testing.html#abi-sys-bus-coresight-devices-dummy-source-n-enable-source)
* [/sys/bus/coresight/devices/dummy\_source<N>/traceid](abi-testing.html#abi-sys-bus-coresight-devices-dummy-source-n-traceid)
* [/sys/bus/coresight/devices/dummy\_source<N>/label](abi-testing.html#abi-sys-bus-coresight-devices-dummy-source-n-label)

## ABI file testing/sysfs-bus-coresight-devices-etb10

Has the following ABI:

* [/sys/bus/coresight/devices/<memory\_map>.etb/enable\_sink](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etb-enable-sink)
* [/sys/bus/coresight/devices/<memory\_map>.etb/trigger\_cntr](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etb-trigger-cntr)
* [/sys/bus/coresight/devices/<memory\_map>.etb/label](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etb-label)
* [/sys/bus/coresight/devices/<memory\_map>.etb/mgmt/rdp](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etb-mgmt-rdp)
* [/sys/bus/coresight/devices/<memory\_map>.etb/mgmt/sts](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etb-mgmt-sts)
* [/sys/bus/coresight/devices/<memory\_map>.etb/mgmt/rrp](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etb-mgmt-rrp)
* [/sys/bus/coresight/devices/<memory\_map>.etb/mgmt/rwp](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etb-mgmt-rwp)
* [/sys/bus/coresight/devices/<memory\_map>.etb/mgmt/trg](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etb-mgmt-trg)
* [/sys/bus/coresight/devices/<memory\_map>.etb/mgmt/ctl](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etb-mgmt-ctl)
* [/sys/bus/coresight/devices/<memory\_map>.etb/mgmt/ffsr](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etb-mgmt-ffsr)
* [/sys/bus/coresight/devices/<memory\_map>.etb/mgmt/ffcr](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etb-mgmt-ffcr)

## ABI file testing/sysfs-bus-coresight-devices-etm3x

Has the following ABI:

* [/sys/bus/coresight/devices/<memory\_map>.[etm|ptm]/enable\_source](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etm-ptm-enable-source)
* [/sys/bus/coresight/devices/<memory\_map>.[etm|ptm]/addr\_idx](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etm-ptm-addr-idx)
* [/sys/bus/coresight/devices/<memory\_map>.[etm|ptm]/addr\_acctype](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etm-ptm-addr-acctype)
* [/sys/bus/coresight/devices/<memory\_map>.[etm|ptm]/addr\_range](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etm-ptm-addr-range)
* [/sys/bus/coresight/devices/<memory\_map>.[etm|ptm]/addr\_single](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etm-ptm-addr-single)
* [/sys/bus/coresight/devices/<memory\_map>.[etm|ptm]/addr\_start](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etm-ptm-addr-start)
* [/sys/bus/coresight/devices/<memory\_map>.[etm|ptm]/addr\_stop](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etm-ptm-addr-stop)
* [/sys/bus/coresight/devices/<memory\_map>.[etm|ptm]/cntr\_idx](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etm-ptm-cntr-idx)
* [/sys/bus/coresight/devices/<memory\_map>.[etm|ptm]/cntr\_event](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etm-ptm-cntr-event)
* [/sys/bus/coresight/devices/<memory\_map>.[etm|ptm]/cntr\_val](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etm-ptm-cntr-val)
* [/sys/bus/coresight/devices/<memory\_map>.[etm|ptm]/cntr\_rld\_val](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etm-ptm-cntr-rld-val)
* [/sys/bus/coresight/devices/<memory\_map>.[etm|ptm]/cntr\_rld\_event](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etm-ptm-cntr-rld-event)
* [/sys/bus/coresight/devices/<memory\_map>.[etm|ptm]/ctxid\_idx](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etm-ptm-ctxid-idx)
* [/sys/bus/coresight/devices/<memory\_map>.[etm|ptm]/ctxid\_mask](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etm-ptm-ctxid-mask)
* [/sys/bus/coresight/devices/<memory\_map>.[etm|ptm]/ctxid\_pid](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etm-ptm-ctxid-pid)
* [/sys/bus/coresight/devices/<memory\_map>.[etm|ptm]/enable\_event](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etm-ptm-enable-event)
* [/sys/bus/coresight/devices/<memory\_map>.[etm|ptm]/etmsr](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etm-ptm-etmsr)
* [/sys/bus/coresight/devices/<memory\_map>.[etm|ptm]/fifofull\_level](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etm-ptm-fifofull-level)
* [/sys/bus/coresight/devices/<memory\_map>.[etm|ptm]/mode](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etm-ptm-mode)
* [/sys/bus/coresight/devices/<memory\_map>.[etm|ptm]/nr\_addr\_cmp](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etm-ptm-nr-addr-cmp)
* [/sys/bus/coresight/devices/<memory\_map>.[etm|ptm]/nr\_cntr](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etm-ptm-nr-cntr)
* [/sys/bus/coresight/devices/<memory\_map>.[etm|ptm]/nr\_ctxid\_cmp](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etm-ptm-nr-ctxid-cmp)
* [/sys/bus/coresight/devices/<memory\_map>.[etm|ptm]/reset](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etm-ptm-reset)
* [/sys/bus/coresight/devices/<memory\_map>.[etm|ptm]/seq\_12\_event](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etm-ptm-seq-12-event)
* [/sys/bus/coresight/devices/<memory\_map>.[etm|ptm]/seq\_13\_event](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etm-ptm-seq-13-event)
* [/sys/bus/coresight/devices/<memory\_map>.[etm|ptm]/seq\_21\_event](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etm-ptm-seq-21-event)
* [/sys/bus/coresight/devices/<memory\_map>.[etm|ptm]/seq\_23\_event](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etm-ptm-seq-23-event)
* [/sys/bus/coresight/devices/<memory\_map>.[etm|ptm]/seq\_31\_event](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etm-ptm-seq-31-event)
* [/sys/bus/coresight/devices/<memory\_map>.[etm|ptm]/seq\_32\_event](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etm-ptm-seq-32-event)
* [/sys/bus/coresight/devices/<memory\_map>.[etm|ptm]/curr\_seq\_state](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etm-ptm-curr-seq-state)
* [/sys/bus/coresight/devices/<memory\_map>.[etm|ptm]/sync\_freq](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etm-ptm-sync-freq)
* [/sys/bus/coresight/devices/<memory\_map>.[etm|ptm]/timestamp\_event](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etm-ptm-timestamp-event)
* [/sys/bus/coresight/devices/<memory\_map>.[etm|ptm]/traceid](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etm-ptm-traceid)
* [/sys/bus/coresight/devices/<memory\_map>.[etm|ptm]/trigger\_event](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etm-ptm-trigger-event)
* [/sys/bus/coresight/devices/<memory\_map>.[etm|ptm]/cpu](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etm-ptm-cpu)
* [/sys/bus/coresight/devices/<memory\_map>.[etm|ptm]/label](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etm-ptm-label)
* [/sys/bus/coresight/devices/<memory\_map>.[etm|ptm]/mgmt/etmccr](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etm-ptm-mgmt-etmccr)
* [/sys/bus/coresight/devices/<memory\_map>.[etm|ptm]/mgmt/etmccer](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etm-ptm-mgmt-etmccer)
* [/sys/bus/coresight/devices/<memory\_map>.[etm|ptm]/mgmt/etmscr](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etm-ptm-mgmt-etmscr)
* [/sys/bus/coresight/devices/<memory\_map>.[etm|ptm]/mgmt/etmidr](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etm-ptm-mgmt-etmidr)
* [/sys/bus/coresight/devices/<memory\_map>.[etm|ptm]/mgmt/etmcr](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etm-ptm-mgmt-etmcr)
* [/sys/bus/coresight/devices/<memory\_map>.[etm|ptm]/mgmt/etmtraceidr](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etm-ptm-mgmt-etmtraceidr)
* [/sys/bus/coresight/devices/<memory\_map>.[etm|ptm]/mgmt/etmteevr](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etm-ptm-mgmt-etmteevr)
* [/sys/bus/coresight/devices/<memory\_map>.[etm|ptm]/mgmt/etmtsscr](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etm-ptm-mgmt-etmtsscr)
* [/sys/bus/coresight/devices/<memory\_map>.[etm|ptm]/mgmt/etmtecr1](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etm-ptm-mgmt-etmtecr1)
* [/sys/bus/coresight/devices/<memory\_map>.[etm|ptm]/mgmt/etmtecr2](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-etm-ptm-mgmt-etmtecr2)

## ABI file testing/sysfs-bus-coresight-devices-etm4x

Has the following ABI:

* [/sys/bus/coresight/devices/etm<N>/enable\_source](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-enable-source)
* [/sys/bus/coresight/devices/etm<N>/cpu](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-cpu)
* [/sys/bus/coresight/devices/etm<N>/nr\_pe\_cmp](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-nr-pe-cmp)
* [/sys/bus/coresight/devices/etm<N>/nr\_addr\_cmp](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-nr-addr-cmp)
* [/sys/bus/coresight/devices/etm<N>/nr\_cntr](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-nr-cntr)
* [/sys/bus/coresight/devices/etm<N>/nr\_ext\_inp](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-nr-ext-inp)
* [/sys/bus/coresight/devices/etm<N>/numcidc](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-numcidc)
* [/sys/bus/coresight/devices/etm<N>/numvmidc](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-numvmidc)
* [/sys/bus/coresight/devices/etm<N>/nrseqstate](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-nrseqstate)
* [/sys/bus/coresight/devices/etm<N>/nr\_resource](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-nr-resource)
* [/sys/bus/coresight/devices/etm<N>/nr\_ss\_cmp](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-nr-ss-cmp)
* [/sys/bus/coresight/devices/etm<N>/reset](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-reset)
* [/sys/bus/coresight/devices/etm<N>/mode](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-mode)
* [/sys/bus/coresight/devices/etm<N>/pe](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-pe)
* [/sys/bus/coresight/devices/etm<N>/event](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-event)
* [/sys/bus/coresight/devices/etm<N>/event\_instren](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-event-instren)
* [/sys/bus/coresight/devices/etm<N>/event\_ts](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-event-ts)
* [/sys/bus/coresight/devices/etm<N>/syncfreq](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-syncfreq)
* [/sys/bus/coresight/devices/etm<N>/cyc\_threshold](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-cyc-threshold)
* [/sys/bus/coresight/devices/etm<N>/bb\_ctrl](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-bb-ctrl)
* [/sys/bus/coresight/devices/etm<N>/event\_vinst](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-event-vinst)
* [/sys/bus/coresight/devices/etm<N>/s\_exlevel\_vinst](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-s-exlevel-vinst)
* [/sys/bus/coresight/devices/etm<N>/ns\_exlevel\_vinst](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-ns-exlevel-vinst)
* [/sys/bus/coresight/devices/etm<N>/addr\_idx](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-addr-idx)
* [/sys/bus/coresight/devices/etm<N>/addr\_instdatatype](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-addr-instdatatype)
* [/sys/bus/coresight/devices/etm<N>/addr\_single](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-addr-single)
* [/sys/bus/coresight/devices/etm<N>/addr\_range](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-addr-range)
* [/sys/bus/coresight/devices/etm<N>/seq\_idx](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-seq-idx)
* [/sys/bus/coresight/devices/etm<N>/seq\_state](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-seq-state)
* [/sys/bus/coresight/devices/etm<N>/seq\_event](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-seq-event)
* [/sys/bus/coresight/devices/etm<N>/seq\_reset\_event](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-seq-reset-event)
* [/sys/bus/coresight/devices/etm<N>/cntr\_idx](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-cntr-idx)
* [/sys/bus/coresight/devices/etm<N>/cntrldvr](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-cntrldvr)
* [/sys/bus/coresight/devices/etm<N>/cntr\_val](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-cntr-val)
* [/sys/bus/coresight/devices/etm<N>/cntr\_ctrl](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-cntr-ctrl)
* [/sys/bus/coresight/devices/etm<N>/res\_idx](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-res-idx)
* [/sys/bus/coresight/devices/etm<N>/res\_ctrl](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-res-ctrl)
* [/sys/bus/coresight/devices/etm<N>/ctxid\_idx](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-ctxid-idx)
* [/sys/bus/coresight/devices/etm<N>/ctxid\_pid](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-ctxid-pid)
* [/sys/bus/coresight/devices/etm<N>/ctxid\_masks](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-ctxid-masks)
* [/sys/bus/coresight/devices/etm<N>/vmid\_idx](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-vmid-idx)
* [/sys/bus/coresight/devices/etm<N>/vmid\_val](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-vmid-val)
* [/sys/bus/coresight/devices/etm<N>/vmid\_masks](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-vmid-masks)
* [/sys/bus/coresight/devices/etm<N>/addr\_exlevel\_s\_ns](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-addr-exlevel-s-ns)
* [/sys/bus/coresight/devices/etm<N>/vinst\_pe\_cmp\_start\_stop](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-vinst-pe-cmp-start-stop)
* [/sys/bus/coresight/devices/etm<N>/addr\_cmp\_view](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-addr-cmp-view)
* [/sys/bus/coresight/devices/etm<N>/sshot\_idx](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-sshot-idx)
* [/sys/bus/coresight/devices/etm<N>/sshot\_ctrl](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-sshot-ctrl)
* [/sys/bus/coresight/devices/etm<N>/sshot\_status](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-sshot-status)
* [/sys/bus/coresight/devices/etm<N>/sshot\_pe\_ctrl](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-sshot-pe-ctrl)
* [/sys/bus/coresight/devices/etm<N>/label](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-label)
* [/sys/bus/coresight/devices/etm<N>/mgmt/trcoslsr](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-mgmt-trcoslsr)
* [/sys/bus/coresight/devices/etm<N>/mgmt/trcpdcr](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-mgmt-trcpdcr)
* [/sys/bus/coresight/devices/etm<N>/mgmt/trcpdsr](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-mgmt-trcpdsr)
* [/sys/bus/coresight/devices/etm<N>/mgmt/trclsr](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-mgmt-trclsr)
* [/sys/bus/coresight/devices/etm<N>/mgmt/trcauthstatus](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-mgmt-trcauthstatus)
* [/sys/bus/coresight/devices/etm<N>/mgmt/trcdevid](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-mgmt-trcdevid)
* [/sys/bus/coresight/devices/etm<N>/mgmt/trcdevarch](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-mgmt-trcdevarch)
* [/sys/bus/coresight/devices/etm<N>/mgmt/trcdevtype](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-mgmt-trcdevtype)
* [/sys/bus/coresight/devices/etm<N>/mgmt/trcpidr0](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-mgmt-trcpidr0)
* [/sys/bus/coresight/devices/etm<N>/mgmt/trcpidr1](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-mgmt-trcpidr1)
* [/sys/bus/coresight/devices/etm<N>/mgmt/trcpidr2](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-mgmt-trcpidr2)
* [/sys/bus/coresight/devices/etm<N>/mgmt/trcpidr3](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-mgmt-trcpidr3)
* [/sys/bus/coresight/devices/etm<N>/mgmt/trcconfig](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-mgmt-trcconfig)
* [/sys/bus/coresight/devices/etm<N>/mgmt/trctraceid](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-mgmt-trctraceid)
* [/sys/bus/coresight/devices/etm<N>/trcidr/trcidr0](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-trcidr-trcidr0)
* [/sys/bus/coresight/devices/etm<N>/trcidr/trcidr1](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-trcidr-trcidr1)
* [/sys/bus/coresight/devices/etm<N>/trcidr/trcidr2](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-trcidr-trcidr2)
* [/sys/bus/coresight/devices/etm<N>/trcidr/trcidr3](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-trcidr-trcidr3)
* [/sys/bus/coresight/devices/etm<N>/trcidr/trcidr4](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-trcidr-trcidr4)
* [/sys/bus/coresight/devices/etm<N>/trcidr/trcidr5](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-trcidr-trcidr5)
* [/sys/bus/coresight/devices/etm<N>/trcidr/trcidr8](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-trcidr-trcidr8)
* [/sys/bus/coresight/devices/etm<N>/trcidr/trcidr9](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-trcidr-trcidr9)
* [/sys/bus/coresight/devices/etm<N>/trcidr/trcidr10](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-trcidr-trcidr10)
* [/sys/bus/coresight/devices/etm<N>/trcidr/trcidr11](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-trcidr-trcidr11)
* [/sys/bus/coresight/devices/etm<N>/trcidr/trcidr12](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-trcidr-trcidr12)
* [/sys/bus/coresight/devices/etm<N>/trcidr/trcidr13](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-trcidr-trcidr13)
* [/sys/bus/coresight/devices/etm<N>/ts\_source](abi-testing.html#abi-sys-bus-coresight-devices-etm-n-ts-source)

## ABI file testing/sysfs-bus-coresight-devices-funnel

Has the following ABI:

* [/sys/bus/coresight/devices/<memory\_map>.funnel/funnel\_ctrl](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-funnel-funnel-ctrl)
* [/sys/bus/coresight/devices/<memory\_map>.funnel/priority](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-funnel-priority)
* [/sys/bus/coresight/devices/<memory\_map>.funnel/label](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-funnel-label)

## ABI file testing/sysfs-bus-coresight-devices-stm

Has the following ABI:

* [/sys/bus/coresight/devices/<memory\_map>.stm/enable\_source](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-stm-enable-source)
* [/sys/bus/coresight/devices/<memory\_map>.stm/hwevent\_enable](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-stm-hwevent-enable)
* [/sys/bus/coresight/devices/<memory\_map>.stm/hwevent\_select](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-stm-hwevent-select)
* [/sys/bus/coresight/devices/<memory\_map>.stm/port\_enable](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-stm-port-enable)
* [/sys/bus/coresight/devices/<memory\_map>.stm/port\_select](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-stm-port-select)
* [/sys/bus/coresight/devices/<memory\_map>.stm/status](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-stm-status)
* [/sys/bus/coresight/devices/<memory\_map>.stm/traceid](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-stm-traceid)
* [/sys/bus/coresight/devices/<memory\_map>.stm/label](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-stm-label)

## ABI file testing/sysfs-bus-coresight-devices-tmc

Has the following ABI:

* [/sys/bus/coresight/devices/<memory\_map>.tmc/trigger\_cntr](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-tmc-trigger-cntr)
* [/sys/bus/coresight/devices/<memory\_map>.tmc/mgmt/rsz](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-tmc-mgmt-rsz)
* [/sys/bus/coresight/devices/<memory\_map>.tmc/mgmt/sts](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-tmc-mgmt-sts)
* [/sys/bus/coresight/devices/<memory\_map>.tmc/mgmt/rrp](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-tmc-mgmt-rrp)
* [/sys/bus/coresight/devices/<memory\_map>.tmc/mgmt/rwp](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-tmc-mgmt-rwp)
* [/sys/bus/coresight/devices/<memory\_map>.tmc/mgmt/trg](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-tmc-mgmt-trg)
* [/sys/bus/coresight/devices/<memory\_map>.tmc/mgmt/ctl](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-tmc-mgmt-ctl)
* [/sys/bus/coresight/devices/<memory\_map>.tmc/mgmt/ffsr](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-tmc-mgmt-ffsr)
* [/sys/bus/coresight/devices/<memory\_map>.tmc/mgmt/ffcr](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-tmc-mgmt-ffcr)
* [/sys/bus/coresight/devices/<memory\_map>.tmc/mgmt/mode](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-tmc-mgmt-mode)
* [/sys/bus/coresight/devices/<memory\_map>.tmc/mgmt/devid](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-tmc-mgmt-devid)
* [/sys/bus/coresight/devices/<memory\_map>.tmc/buffer\_size](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-tmc-buffer-size)
* [/sys/bus/coresight/devices/<memory\_map>.tmc/buf\_modes\_available](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-tmc-buf-modes-available)
* [/sys/bus/coresight/devices/<memory\_map>.tmc/buf\_mode\_preferred](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-tmc-buf-mode-preferred)
* [/sys/bus/coresight/devices/<memory\_map>.tmc/label](abi-testing.html#abi-sys-bus-coresight-devices-memory-map-tmc-label)

## ABI file testing/sysfs-bus-coresight-devices-tpda

Has the following ABI:

* [/sys/bus/coresight/devices/<tpda-name>/trig\_async\_enable](abi-testing.html#abi-sys-bus-coresight-devices-tpda-name-trig-async-enable)
* [/sys/bus/coresight/devices/<tpda-name>/trig\_flag\_ts\_enable](abi-testing.html#abi-sys-bus-coresight-devices-tpda-name-trig-flag-ts-enable)
* [/sys/bus/coresight/devices/<tpda-name>/trig\_freq\_enable](abi-testing.html#abi-sys-bus-coresight-devices-tpda-name-trig-freq-enable)
* [/sys/bus/coresight/devices/<tpda-name>/freq\_ts\_enable](abi-testing.html#abi-sys-bus-coresight-devices-tpda-name-freq-ts-enable)
* [/sys/bus/coresight/devices/<tpda-name>/cmbchan\_mode](abi-testing.html#abi-sys-bus-coresight-devices-tpda-name-cmbchan-mode)
* [/sys/bus/coresight/devices/<tpda-name>/global\_flush\_req](abi-testing.html#abi-sys-bus-coresight-devices-tpda-name-global-flush-req)
* [/sys/bus/coresight/devices/<tpda-name>/syncr\_mode](abi-testing.html#abi-sys-bus-coresight-devices-tpda-name-syncr-mode)
* [/sys/bus/coresight/devices/<tpda-name>/syncr\_count](abi-testing.html#abi-sys-bus-coresight-devices-tpda-name-syncr-count)
* [/sys/bus/coresight/devices/<tpda-name>/port\_flush\_req](abi-testing.html#abi-sys-bus-coresight-devices-tpda-name-port-flush-req)

## ABI file testing/sysfs-bus-coresight-devices-tpdm

Has the following ABI:

* [/sys/bus/coresight/devices/<tpdm-name>/integration\_test](abi-testing.html#abi-sys-bus-coresight-devices-tpdm-name-integration-test)
* [/sys/bus/coresight/devices/<tpdm-name>/reset\_dataset](abi-testing.html#abi-sys-bus-coresight-devices-tpdm-name-reset-dataset)
* [/sys/bus/coresight/devices/<tpdm-name>/dsb\_trig\_type](abi-testing.html#abi-sys-bus-coresight-devices-tpdm-name-dsb-trig-type)
* [/sys/bus/coresight/devices/<tpdm-name>/dsb\_trig\_ts](abi-testing.html#abi-sys-bus-coresight-devices-tpdm-name-dsb-trig-ts)
* [/sys/bus/coresight/devices/<tpdm-name>/dsb\_mode](abi-testing.html#abi-sys-bus-coresight-devices-tpdm-name-dsb-mode)
* [/sys/bus/coresight/devices/<tpdm-name>/dsb\_edge/ctrl\_idx](abi-testing.html#abi-sys-bus-coresight-devices-tpdm-name-dsb-edge-ctrl-idx)
* [/sys/bus/coresight/devices/<tpdm-name>/dsb\_edge/ctrl\_val](abi-testing.html#abi-sys-bus-coresight-devices-tpdm-name-dsb-edge-ctrl-val)
* [/sys/bus/coresight/devices/<tpdm-name>/dsb\_edge/ctrl\_mask](abi-testing.html#abi-sys-bus-coresight-devices-tpdm-name-dsb-edge-ctrl-mask)
* [/sys/bus/coresight/devices/<tpdm-name>/dsb\_edge/edcr[0:15]](abi-testing.html#abi-sys-bus-coresight-devices-tpdm-name-dsb-edge-edcr-0-15)
* [/sys/bus/coresight/devices/<tpdm-name>/dsb\_edge/edcmr[0:7]](abi-testing.html#abi-sys-bus-coresight-devices-tpdm-name-dsb-edge-edcmr-0-7)
* [/sys/bus/coresight/devices/<tpdm-name>/dsb\_trig\_patt/xpr[0:7]](abi-testing.html#abi-sys-bus-coresight-devices-tpdm-name-dsb-trig-patt-xpr-0-7)
* [/sys/bus/coresight/devices/<tpdm-name>/dsb\_trig\_patt/xpmr[0:7]](abi-testing.html#abi-sys-bus-coresight-devices-tpdm-name-dsb-trig-patt-xpmr-0-7)
* [/sys/bus/coresight/devices/<tpdm-name>/dsb\_patt/tpr[0:7]](abi-testing.html#abi-sys-bus-coresight-devices-tpdm-name-dsb-patt-tpr-0-7)
* [/sys/bus/coresight/devices/<tpdm-name>/dsb\_patt/tpmr[0:7]](abi-testing.html#abi-sys-bus-coresight-devices-tpdm-name-dsb-patt-tpmr-0-7)
* [/sys/bus/coresight/devices/<tpdm-name>/dsb\_patt/enable\_ts](abi-testing.html#abi-sys-bus-coresight-devices-tpdm-name-dsb-patt-enable-ts)
* [/sys/bus/coresight/devices/<tpdm-name>/dsb\_patt/set\_type](abi-testing.html#abi-sys-bus-coresight-devices-tpdm-name-dsb-patt-set-type)
* [/sys/bus/coresight/devices/<tpdm-name>/dsb\_msr/msr[0:31]](abi-testing.html#abi-sys-bus-coresight-devices-tpdm-name-dsb-msr-msr-0-31)
* [/sys/bus/coresight/devices/<tpdm-name>/cmb\_mode](abi-testing.html#abi-sys-bus-coresight-devices-tpdm-name-cmb-mode)
* [/sys/bus/coresight/devices/<tpdm-name>/cmb\_trig\_patt/xpr[0:1]](abi-testing.html#abi-sys-bus-coresight-devices-tpdm-name-cmb-trig-patt-xpr-0-1)
* [/sys/bus/coresight/devices/<tpdm-name>/cmb\_trig\_patt/xpmr[0:1]](abi-testing.html#abi-sys-bus-coresight-devices-tpdm-name-cmb-trig-patt-xpmr-0-1)
* [/sys/bus/coresight/devices/<tpdm-name>/dsb\_patt/tpr[0:1]](abi-testing.html#abi-sys-bus-coresight-devices-tpdm-name-dsb-patt-tpr-0-1)
* [/sys/bus/coresight/devices/<tpdm-name>/dsb\_patt/tpmr[0:1]](abi-testing.html#abi-sys-bus-coresight-devices-tpdm-name-dsb-patt-tpmr-0-1)
* [/sys/bus/coresight/devices/<tpdm-name>/cmb\_patt/enable\_ts](abi-testing.html#abi-sys-bus-coresight-devices-tpdm-name-cmb-patt-enable-ts)
* [/sys/bus/coresight/devices/<tpdm-name>/cmb\_trig\_ts](abi-testing.html#abi-sys-bus-coresight-devices-tpdm-name-cmb-trig-ts)
* [/sys/bus/coresight/devices/<tpdm-name>/cmb\_ts\_all](abi-testing.html#abi-sys-bus-coresight-devices-tpdm-name-cmb-ts-all)
* [/sys/bus/coresight/devices/<tpdm-name>/cmb\_msr/msr[0:31]](abi-testing.html#abi-sys-bus-coresight-devices-tpdm-name-cmb-msr-msr-0-31)
* [/sys/bus/coresight/devices/<tpdm-name>/mcmb\_trig\_lane](abi-testing.html#abi-sys-bus-coresight-devices-tpdm-name-mcmb-trig-lane)
* [/sys/bus/coresight/devices/<tpdm-name>/mcmb\_lanes\_select](abi-testing.html#abi-sys-bus-coresight-devices-tpdm-name-mcmb-lanes-select)
* [/sys/bus/coresight/devices/<tpdm-name>/label](abi-testing.html#abi-sys-bus-coresight-devices-tpdm-name-label)
* [/sys/bus/coresight/devices/<tpdm-name>/traceid](abi-testing.html#abi-sys-bus-coresight-devices-tpdm-name-traceid)

## ABI file testing/sysfs-bus-coresight-devices-trbe

Has the following ABI:

* [/sys/bus/coresight/devices/trbe<cpu>/align](abi-testing.html#abi-sys-bus-coresight-devices-trbe-cpu-align)
* [/sys/bus/coresight/devices/trbe<cpu>/flag](abi-testing.html#abi-sys-bus-coresight-devices-trbe-cpu-flag)
* [/sys/bus/coresight/devices/trbe<cpu>/label](abi-testing.html#abi-sys-bus-coresight-devices-trbe-cpu-label)

## ABI file testing/sysfs-bus-coresight-devices-ultra\_smb

Has the following ABI:

* [/sys/bus/coresight/devices/ultra\_smb<N>/enable\_sink](abi-testing.html#abi-sys-bus-coresight-devices-ultra-smb-n-enable-sink)
* [/sys/bus/coresight/devices/ultra\_smb<N>/mgmt/buf\_size](abi-testing.html#abi-sys-bus-coresight-devices-ultra-smb-n-mgmt-buf-size)
* [/sys/bus/coresight/devices/ultra\_smb<N>/mgmt/buf\_status](abi-testing.html#abi-sys-bus-coresight-devices-ultra-smb-n-mgmt-buf-status)
* [/sys/bus/coresight/devices/ultra\_smb<N>/mgmt/read\_pos](abi-testing.html#abi-sys-bus-coresight-devices-ultra-smb-n-mgmt-read-pos)
* [/sys/bus/coresight/devices/ultra\_smb<N>/mgmt/write\_pos](abi-testing.html#abi-sys-bus-coresight-devices-ultra-smb-n-mgmt-write-pos)

## ABI file testing/sysfs-bus-counter

Has the following ABI:

* [/sys/bus/counter/devices/counterX/cascade\_counts\_enable](abi-testing.html#abi-sys-bus-counter-devices-counterx-cascade-counts-enable)
* [/sys/bus/counter/devices/counterX/external\_input\_phase\_clock\_select](abi-testing.html#abi-sys-bus-counter-devices-counterx-external-input-phase-clock-select)
* [/sys/bus/counter/devices/counterX/external\_input\_phase\_clock\_select\_available](abi-testing.html#abi-sys-bus-counter-devices-counterx-external-input-phase-clock-select-available)
* [/sys/bus/counter/devices/counterX/countY/count](abi-testing.html#abi-sys-bus-counter-devices-counterx-county-count)
* [/sys/bus/counter/devices/counterX/countY/compare](abi-testing.html#abi-sys-bus-counter-devices-counterx-county-compare)
* [/sys/bus/counter/devices/counterX/countY/capture](abi-testing.html#abi-sys-bus-counter-devices-counterx-county-capture)
* [/sys/bus/counter/devices/counterX/countY/ceiling](abi-testing.html#abi-sys-bus-counter-devices-counterx-county-ceiling)
* [/sys/bus/counter/devices/counterX/countY/floor](abi-testing.html#abi-sys-bus-counter-devices-counterx-county-floor)
* [/sys/bus/counter/devices/counterX/countY/count\_mode](abi-testing.html#abi-sys-bus-counter-devices-counterx-county-count-mode)
* [/sys/bus/counter/devices/counterX/countY/count\_mode\_available](abi-testing.html#abi-sys-bus-counter-devices-counterx-county-count-mode-available)
* [/sys/bus/counter/devices/counterX/countY/error\_noise\_available](abi-testing.html#abi-sys-bus-counter-devices-counterx-county-count-mode-available)
* [/sys/bus/counter/devices/counterX/countY/function\_available](abi-testing.html#abi-sys-bus-counter-devices-counterx-county-count-mode-available)
* [/sys/bus/counter/devices/counterX/countY/prescaler\_available](abi-testing.html#abi-sys-bus-counter-devices-counterx-county-count-mode-available)
* [/sys/bus/counter/devices/counterX/countY/signalZ\_action\_available](abi-testing.html#abi-sys-bus-counter-devices-counterx-county-count-mode-available)
* [/sys/bus/counter/devices/counterX/countY/direction](abi-testing.html#abi-sys-bus-counter-devices-counterx-county-direction)
* [/sys/bus/counter/devices/counterX/countY/enable](abi-testing.html#abi-sys-bus-counter-devices-counterx-county-enable)
* [/sys/bus/counter/devices/counterX/countY/error\_noise](abi-testing.html#abi-sys-bus-counter-devices-counterx-county-error-noise)
* [/sys/bus/counter/devices/counterX/countY/function](abi-testing.html#abi-sys-bus-counter-devices-counterx-county-function)
* [/sys/bus/counter/devices/counterX/countY/name](abi-testing.html#abi-sys-bus-counter-devices-counterx-county-name)
* [/sys/bus/counter/devices/counterX/countY/prescaler](abi-testing.html#abi-sys-bus-counter-devices-counterx-county-prescaler)
* [/sys/bus/counter/devices/counterX/countY/preset](abi-testing.html#abi-sys-bus-counter-devices-counterx-county-preset)
* [/sys/bus/counter/devices/counterX/countY/preset\_enable](abi-testing.html#abi-sys-bus-counter-devices-counterx-county-preset-enable)
* [/sys/bus/counter/devices/counterX/countY/signalZ\_action](abi-testing.html#abi-sys-bus-counter-devices-counterx-county-signalz-action)
* [/sys/bus/counter/devices/counterX/countY/num\_overflows](abi-testing.html#abi-sys-bus-counter-devices-counterx-county-num-overflows)
* [/sys/bus/counter/devices/counterX/cascade\_counts\_enable\_component\_id](abi-testing.html#abi-sys-bus-counter-devices-counterx-cascade-counts-enable-component-id)
* [/sys/bus/counter/devices/counterX/external\_input\_phase\_clock\_select\_component\_id](abi-testing.html#abi-sys-bus-counter-devices-counterx-cascade-counts-enable-component-id)
* [/sys/bus/counter/devices/counterX/countY/capture\_component\_id](abi-testing.html#abi-sys-bus-counter-devices-counterx-cascade-counts-enable-component-id)
* [/sys/bus/counter/devices/counterX/countY/ceiling\_component\_id](abi-testing.html#abi-sys-bus-counter-devices-counterx-cascade-counts-enable-component-id)
* [/sys/bus/counter/devices/counterX/countY/compare\_component\_id](abi-testing.html#abi-sys-bus-counter-devices-counterx-cascade-counts-enable-component-id)
* [/sys/bus/counter/devices/counterX/countY/count\_mode\_component\_id](abi-testing.html#abi-sys-bus-counter-devices-counterx-cascade-counts-enable-component-id)
* [/sys/bus/counter/devices/counterX/countY/direction\_component\_id](abi-testing.html#abi-sys-bus-counter-devices-counterx-cascade-counts-enable-component-id)
* [/sys/bus/counter/devices/counterX/countY/enable\_component\_id](abi-testing.html#abi-sys-bus-counter-devices-counterx-cascade-counts-enable-component-id)
* [/sys/bus/counter/devices/counterX/countY/error\_noise\_component\_id](abi-testing.html#abi-sys-bus-counter-devices-counterx-cascade-counts-enable-component-id)
* [/sys/bus/counter/devices/counterX/countY/floor\_component\_id](abi-testing.html#abi-sys-bus-counter-devices-counterx-cascade-counts-enable-component-id)
* [/sys/bus/counter/devices/counterX/countY/num\_overflows\_component\_id](abi-testing.html#abi-sys-bus-counter-devices-counterx-cascade-counts-enable-component-id)
* [/sys/bus/counter/devices/counterX/countY/prescaler\_component\_id](abi-testing.html#abi-sys-bus-counter-devices-counterx-cascade-counts-enable-component-id)
* [/sys/bus/counter/devices/counterX/countY/preset\_component\_id](abi-testing.html#abi-sys-bus-counter-devices-counterx-cascade-counts-enable-component-id)
* [/sys/bus/counter/devices/counterX/countY/preset\_enable\_component\_id](abi-testing.html#abi-sys-bus-counter-devices-counterx-cascade-counts-enable-component-id)
* [/sys/bus/counter/devices/counterX/countY/signalZ\_action\_component\_id](abi-testing.html#abi-sys-bus-counter-devices-counterx-cascade-counts-enable-component-id)
* [/sys/bus/counter/devices/counterX/signalY/cable\_fault\_component\_id](abi-testing.html#abi-sys-bus-counter-devices-counterx-cascade-counts-enable-component-id)
* [/sys/bus/counter/devices/counterX/signalY/cable\_fault\_enable\_component\_id](abi-testing.html#abi-sys-bus-counter-devices-counterx-cascade-counts-enable-component-id)
* [/sys/bus/counter/devices/counterX/signalY/filter\_clock\_prescaler\_component\_id](abi-testing.html#abi-sys-bus-counter-devices-counterx-cascade-counts-enable-component-id)
* [/sys/bus/counter/devices/counterX/signalY/frequency\_component\_id](abi-testing.html#abi-sys-bus-counter-devices-counterx-cascade-counts-enable-component-id)
* [/sys/bus/counter/devices/counterX/signalY/index\_polarity\_component\_id](abi-testing.html#abi-sys-bus-counter-devices-counterx-cascade-counts-enable-component-id)
* [/sys/bus/counter/devices/counterX/signalY/polarity\_component\_id](abi-testing.html#abi-sys-bus-counter-devices-counterx-cascade-counts-enable-component-id)
* [/sys/bus/counter/devices/counterX/signalY/synchronous\_mode\_component\_id](abi-testing.html#abi-sys-bus-counter-devices-counterx-cascade-counts-enable-component-id)
* [/sys/bus/counter/devices/counterX/countY/spike\_filter\_ns](abi-testing.html#abi-sys-bus-counter-devices-counterx-county-spike-filter-ns)
* [/sys/bus/counter/devices/counterX/events\_queue\_size](abi-testing.html#abi-sys-bus-counter-devices-counterx-events-queue-size)
* [/sys/bus/counter/devices/counterX/name](abi-testing.html#abi-sys-bus-counter-devices-counterx-name)
* [/sys/bus/counter/devices/counterX/num\_counts](abi-testing.html#abi-sys-bus-counter-devices-counterx-num-counts)
* [/sys/bus/counter/devices/counterX/num\_signals](abi-testing.html#abi-sys-bus-counter-devices-counterx-num-signals)
* [/sys/bus/counter/devices/counterX/signalY/cable\_fault](abi-testing.html#abi-sys-bus-counter-devices-counterx-signaly-cable-fault)
* [/sys/bus/counter/devices/counterX/signalY/cable\_fault\_enable](abi-testing.html#abi-sys-bus-counter-devices-counterx-signaly-cable-fault-enable)
* [/sys/bus/counter/devices/counterX/signalY/filter\_clock\_prescaler](abi-testing.html#abi-sys-bus-counter-devices-counterx-signaly-filter-clock-prescaler)
* [/sys/bus/counter/devices/counterX/signalY/index\_polarity](abi-testing.html#abi-sys-bus-counter-devices-counterx-signaly-index-polarity)
* [/sys/bus/counter/devices/counterX/signalY/index\_polarity\_available](abi-testing.html#abi-sys-bus-counter-devices-counterx-signaly-index-polarity-available)
* [/sys/bus/counter/devices/counterX/signalY/synchronous\_mode\_available](abi-testing.html#abi-sys-bus-counter-devices-counterx-signaly-index-polarity-available)
* [/sys/bus/counter/devices/counterX/signalY/polarity](abi-testing.html#abi-sys-bus-counter-devices-counterx-signaly-polarity)
* [/sys/bus/counter/devices/counterX/signalY/name](abi-testing.html#abi-sys-bus-counter-devices-counterx-signaly-name)
* [/sys/bus/counter/devices/counterX/signalY/signal](abi-testing.html#abi-sys-bus-counter-devices-counterx-signaly-signal)
* [/sys/bus/counter/devices/counterX/signalY/synchronous\_mode](abi-testing.html#abi-sys-bus-counter-devices-counterx-signaly-synchronous-mode)
* [/sys/bus/counter/devices/counterX/signalY/frequency](abi-testing.html#abi-sys-bus-counter-devices-counterx-signaly-frequency)

## ABI file testing/sysfs-bus-css

Has the following ABI:

* [/sys/bus/css/devices/.../type](abi-testing.html#abi-sys-bus-css-devices-type)
* [/sys/bus/css/devices/.../modalias](abi-testing.html#abi-sys-bus-css-devices-modalias)
* [/sys/bus/css/drivers/io\_subchannel/.../chpids](abi-testing.html#abi-sys-bus-css-drivers-io-subchannel-chpids)
* [/sys/bus/css/drivers/io\_subchannel/.../pimpampom](abi-testing.html#abi-sys-bus-css-drivers-io-subchannel-pimpampom)
* [/sys/bus/css/devices/.../driver\_override](abi-testing.html#abi-sys-bus-css-devices-driver-override)

## ABI file testing/sysfs-bus-cxl

Has the following ABI:

* [/sys/bus/cxl/flush](abi-testing.html#abi-sys-bus-cxl-flush)
* [/sys/bus/cxl/devices/memX/firmware\_version](abi-testing.html#abi-sys-bus-cxl-devices-memx-firmware-version)
* [/sys/bus/cxl/devices/memX/payload\_max](abi-testing.html#abi-sys-bus-cxl-devices-memx-payload-max)
* [/sys/bus/cxl/devices/memX/label\_storage\_size](abi-testing.html#abi-sys-bus-cxl-devices-memx-label-storage-size)
* [/sys/bus/cxl/devices/memX/ram/size](abi-testing.html#abi-sys-bus-cxl-devices-memx-ram-size)
* [/sys/bus/cxl/devices/memX/ram/qos\_class](abi-testing.html#abi-sys-bus-cxl-devices-memx-ram-qos-class)
* [/sys/bus/cxl/devices/memX/pmem/size](abi-testing.html#abi-sys-bus-cxl-devices-memx-pmem-size)
* [/sys/bus/cxl/devices/memX/pmem/qos\_class](abi-testing.html#abi-sys-bus-cxl-devices-memx-pmem-qos-class)
* [/sys/bus/cxl/devices/memX/serial](abi-testing.html#abi-sys-bus-cxl-devices-memx-serial)
* [/sys/bus/cxl/devices/memX/numa\_node](abi-testing.html#abi-sys-bus-cxl-devices-memx-numa-node)
* [/sys/bus/cxl/devices/memX/security/state](abi-testing.html#abi-sys-bus-cxl-devices-memx-security-state)
* [/sys/bus/cxl/devices/memX/security/sanitize](abi-testing.html#abi-sys-bus-cxl-devices-memx-security-sanitize)
* [/sys/bus/cxl/devices/memX/firmware/](abi-testing.html#abi-sys-bus-cxl-devices-memx-firmware)
* [/sys/bus/cxl/devices/\*/devtype](abi-testing.html#abi-sys-bus-cxl-devices-devtype)
* [/sys/bus/cxl/devices/\*/modalias](abi-testing.html#abi-sys-bus-cxl-devices-modalias)
* [/sys/bus/cxl/devices/portX/uport](abi-testing.html#abi-sys-bus-cxl-devices-portx-uport)
* [/sys/bus/cxl/devices/{port,endpoint}X/parent\_dport](abi-testing.html#abi-sys-bus-cxl-devices-port-endpoint-x-parent-dport)
* [/sys/bus/cxl/devices/portX/dportY](abi-testing.html#abi-sys-bus-cxl-devices-portx-dporty)
* [/sys/bus/cxl/devices/portX/decoders\_committed](abi-testing.html#abi-sys-bus-cxl-devices-portx-decoders-committed)
* [/sys/bus/cxl/devices/decoderX.Y](abi-testing.html#abi-sys-bus-cxl-devices-decoderx-y)
* [/sys/bus/cxl/devices/decoderX.Y/{start,size}](abi-testing.html#abi-sys-bus-cxl-devices-decoderx-y-start-size)
* [/sys/bus/cxl/devices/decoderX.Y/locked](abi-testing.html#abi-sys-bus-cxl-devices-decoderx-y-locked)
* [/sys/bus/cxl/devices/decoderX.Y/target\_list](abi-testing.html#abi-sys-bus-cxl-devices-decoderx-y-target-list)
* [/sys/bus/cxl/devices/decoderX.Y/cap\_{pmem,ram,type2,type3}](abi-testing.html#abi-sys-bus-cxl-devices-decoderx-y-cap-pmem-ram-type2-type3)
* [/sys/bus/cxl/devices/decoderX.Y/target\_type](abi-testing.html#abi-sys-bus-cxl-devices-decoderx-y-target-type)
* [/sys/bus/cxl/devices/endpointX/CDAT](abi-testing.html#abi-sys-bus-cxl-devices-endpointx-cdat)
* [/sys/bus/cxl/devices/decoderX.Y/mode](abi-testing.html#abi-sys-bus-cxl-devices-decoderx-y-mode)
* [/sys/bus/cxl/devices/decoderX.Y/dpa\_resource](abi-testing.html#abi-sys-bus-cxl-devices-decoderx-y-dpa-resource)
* [/sys/bus/cxl/devices/decoderX.Y/dpa\_size](abi-testing.html#abi-sys-bus-cxl-devices-decoderx-y-dpa-size)
* [/sys/bus/cxl/devices/decoderX.Y/interleave\_ways](abi-testing.html#abi-sys-bus-cxl-devices-decoderx-y-interleave-ways)
* [/sys/bus/cxl/devices/decoderX.Y/interleave\_granularity](abi-testing.html#abi-sys-bus-cxl-devices-decoderx-y-interleave-granularity)
* [/sys/bus/cxl/devices/decoderX.Y/create\_{pmem,ram}\_region](abi-testing.html#abi-sys-bus-cxl-devices-decoderx-y-create-pmem-ram-region)
* [/sys/bus/cxl/devices/decoderX.Y/delete\_region](abi-testing.html#abi-sys-bus-cxl-devices-decoderx-y-delete-region)
* [/sys/bus/cxl/devices/decoderX.Y/qos\_class](abi-testing.html#abi-sys-bus-cxl-devices-decoderx-y-qos-class)
* [/sys/bus/cxl/devices/regionZ/uuid](abi-testing.html#abi-sys-bus-cxl-devices-regionz-uuid)
* [/sys/bus/cxl/devices/regionZ/interleave\_granularity](abi-testing.html#abi-sys-bus-cxl-devices-regionz-interleave-granularity)
* [/sys/bus/cxl/devices/regionZ/interleave\_ways](abi-testing.html#abi-sys-bus-cxl-devices-regionz-interleave-ways)
* [/sys/bus/cxl/devices/regionZ/size](abi-testing.html#abi-sys-bus-cxl-devices-regionz-size)
* [/sys/bus/cxl/devices/regionZ/extended\_linear\_cache\_size](abi-testing.html#abi-sys-bus-cxl-devices-regionz-extended-linear-cache-size)
* [/sys/bus/cxl/devices/regionZ/locked](abi-testing.html#abi-sys-bus-cxl-devices-regionz-locked)
* [/sys/bus/cxl/devices/regionZ/mode](abi-testing.html#abi-sys-bus-cxl-devices-regionz-mode)
* [/sys/bus/cxl/devices/regionZ/resource](abi-testing.html#abi-sys-bus-cxl-devices-regionz-resource)
* [/sys/bus/cxl/devices/regionZ/target[0..N]](abi-testing.html#abi-sys-bus-cxl-devices-regionz-target-0-n)
* [/sys/bus/cxl/devices/regionZ/commit](abi-testing.html#abi-sys-bus-cxl-devices-regionz-commit)
* [/sys/bus/cxl/devices/memX/trigger\_poison\_list](abi-testing.html#abi-sys-bus-cxl-devices-memx-trigger-poison-list)
* [/sys/bus/cxl/devices/regionZ/accessY/read\_bandwidth](abi-testing.html#abi-sys-bus-cxl-devices-regionz-accessy-read-bandwidth)
* [/sys/bus/cxl/devices/regionZ/accessY/read\_latency](abi-testing.html#abi-sys-bus-cxl-devices-regionz-accessy-read-latency)
* [/sys/bus/cxl/devices/nvdimm-bridge0/ndbusX/nmemY/cxl/dirty\_shutdown](abi-testing.html#abi-sys-bus-cxl-devices-nvdimm-bridge0-ndbusx-nmemy-cxl-dirty-shutdown)

## ABI file testing/sysfs-bus-dax

Has the following ABI:

* [/sys/bus/dax/devices/daxX.Y/align](abi-testing.html#abi-sys-bus-dax-devices-daxx-y-align)
* [/sys/bus/dax/devices/daxX.Y/mapping](abi-testing.html#abi-sys-bus-dax-devices-daxx-y-mapping)
* [/sys/bus/dax/devices/daxX.Y/mapping[0..N]/start](abi-testing.html#abi-sys-bus-dax-devices-daxx-y-mapping-0-n-start)
* [/sys/bus/dax/devices/daxX.Y/mapping[0..N]/end](abi-testing.html#abi-sys-bus-dax-devices-daxx-y-mapping-0-n-start)
* [/sys/bus/dax/devices/daxX.Y/mapping[0..N]/page\_offset](abi-testing.html#abi-sys-bus-dax-devices-daxx-y-mapping-0-n-start)
* [/sys/bus/dax/devices/daxX.Y/resource](abi-testing.html#abi-sys-bus-dax-devices-daxx-y-resource)
* [/sys/bus/dax/devices/daxX.Y/size](abi-testing.html#abi-sys-bus-dax-devices-daxx-y-size)
* [/sys/bus/dax/devices/daxX.Y/numa\_node](abi-testing.html#abi-sys-bus-dax-devices-daxx-y-numa-node)
* [/sys/bus/dax/devices/daxX.Y/target\_node](abi-testing.html#abi-sys-bus-dax-devices-daxx-y-target-node)
* [$(readlink -f /sys/bus/dax/devices/daxX.Y)/../dax\_region/available\_size](abi-testing.html#abi-readlink-f-sys-bus-dax-devices-daxx-y-dax-region-available-size)
* [$(readlink -f /sys/bus/dax/devices/daxX.Y)/../dax\_region/size](abi-testing.html#abi-readlink-f-sys-bus-dax-devices-daxx-y-dax-region-size)
* [$(readlink -f /sys/bus/dax/devices/daxX.Y)/../dax\_region/align](abi-testing.html#abi-readlink-f-sys-bus-dax-devices-daxx-y-dax-region-align)
* [$(readlink -f /sys/bus/dax/devices/daxX.Y)/../dax\_region/seed](abi-testing.html#abi-readlink-f-sys-bus-dax-devices-daxx-y-dax-region-seed)
* [$(readlink -f /sys/bus/dax/devices/daxX.Y)/../dax\_region/create](abi-testing.html#abi-readlink-f-sys-bus-dax-devices-daxx-y-dax-region-create)
* [$(readlink -f /sys/bus/dax/devices/daxX.Y)/../dax\_region/delete](abi-testing.html#abi-readlink-f-sys-bus-dax-devices-daxx-y-dax-region-delete)
* [$(readlink -f /sys/bus/dax/devices/daxX.Y)/../dax\_region/id](abi-testing.html#abi-readlink-f-sys-bus-dax-devices-daxx-y-dax-region-id)
* [/sys/bus/dax/devices/daxX.Y/memmap\_on\_memory](abi-testing.html#abi-sys-bus-dax-devices-daxx-y-memmap-on-memory)

## ABI file testing/sysfs-bus-dfl

Has the following ABI:

* [/sys/bus/dfl/devices/dfl\_dev.X/type](abi-testing.html#abi-sys-bus-dfl-devices-dfl-dev-x-type)
* [/sys/bus/dfl/devices/dfl\_dev.X/feature\_id](abi-testing.html#abi-sys-bus-dfl-devices-dfl-dev-x-feature-id)

## ABI file testing/sysfs-bus-dfl-devices-emif

Has the following ABI:

* [/sys/bus/dfl/devices/dfl\_dev.X/infX\_cal\_fail](abi-testing.html#abi-sys-bus-dfl-devices-dfl-dev-x-infx-cal-fail)
* [/sys/bus/dfl/devices/dfl\_dev.X/infX\_init\_done](abi-testing.html#abi-sys-bus-dfl-devices-dfl-dev-x-infx-init-done)
* [/sys/bus/dfl/devices/dfl\_dev.X/infX\_clear](abi-testing.html#abi-sys-bus-dfl-devices-dfl-dev-x-infx-clear)

## ABI file testing/sysfs-bus-dfl-devices-n3000-nios

Has the following ABI:

* [/sys/bus/dfl/devices/dfl\_dev.X/fec\_mode](abi-testing.html#abi-sys-bus-dfl-devices-dfl-dev-x-fec-mode)
* [/sys/bus/dfl/devices/dfl\_dev.X/retimer\_A\_mode](abi-testing.html#abi-sys-bus-dfl-devices-dfl-dev-x-retimer-a-mode)
* [/sys/bus/dfl/devices/dfl\_dev.X/retimer\_B\_mode](abi-testing.html#abi-sys-bus-dfl-devices-dfl-dev-x-retimer-b-mode)
* [/sys/bus/dfl/devices/dfl\_dev.X/nios\_fw\_version](abi-testing.html#abi-sys-bus-dfl-devices-dfl-dev-x-nios-fw-version)

## ABI file testing/sysfs-bus-event\_source-devices

Has the following ABI:

* [/sys/bus/event\_source/devices/<pmu>](abi-testing.html#abi-sys-bus-event-source-devices-pmu)

## ABI file testing/sysfs-bus-event\_source-devices-caps

Has the following ABI:

* [/sys/bus/event\_source/devices/<dev>/caps](abi-testing.html#abi-sys-bus-event-source-devices-dev-caps)

## ABI file testing/sysfs-bus-event\_source-devices-dfl\_fme

Has the following ABI:

* [/sys/bus/event\_source/devices/dfl\_fmeX/format](abi-testing.html#abi-sys-bus-event-source-devices-dfl-fmex-format)
* [/sys/bus/event\_source/devices/dfl\_fmeX/cpumask](abi-testing.html#abi-sys-bus-event-source-devices-dfl-fmex-cpumask)
* [/sys/bus/event\_source/devices/dfl\_fmeX/events](abi-testing.html#abi-sys-bus-event-source-devices-dfl-fmex-events)

## ABI file testing/sysfs-bus-event\_source-devices-dsa

Has the following ABI:

* [/sys/bus/event\_source/devices/dsa\*/format](abi-testing.html#abi-sys-bus-event-source-devices-dsa-format)
* [/sys/bus/event\_source/devices/dsa\*/cpumask](abi-testing.html#abi-sys-bus-event-source-devices-dsa-cpumask)

## ABI file testing/sysfs-bus-event\_source-devices-events

Has the following ABI:

* [/sys/devices/cpu/events/](abi-testing.html#abi-sys-devices-cpu-events)
* [/sys/bus/event\_source/devices/<pmu>/events/<event>](abi-testing.html#abi-sys-bus-event-source-devices-pmu-events-event)
* [/sys/bus/event\_source/devices/<pmu>/events/<event>.unit](abi-testing.html#abi-sys-bus-event-source-devices-pmu-events-event-unit)
* [/sys/bus/event\_source/devices/<pmu>/events/<event>.scale](abi-testing.html#abi-sys-bus-event-source-devices-pmu-events-event-scale)

## ABI file testing/sysfs-bus-event\_source-devices-format

Has the following ABI:

* [/sys/bus/event\_source/devices/<dev>/format](abi-testing.html#abi-sys-bus-event-source-devices-dev-format)

## ABI file testing/sysfs-bus-event\_source-devices-hisi\_ptt

Has the following ABI:

* [/sys/bus/event\_source/devices/hisi\_ptt<sicl\_id>\_<core\_id>/tune](abi-testing.html#abi-sys-bus-event-source-devices-hisi-ptt-sicl-id-core-id-tune)
* [/sys/bus/event\_source/devices/hisi\_ptt<sicl\_id>\_<core\_id>/tune/qos\_tx\_cpl](abi-testing.html#abi-sys-bus-event-source-devices-hisi-ptt-sicl-id-core-id-tune-qos-tx-cpl)
* [/sys/bus/event\_source/devices/hisi\_ptt<sicl\_id>\_<core\_id>/tune/qos\_tx\_np](abi-testing.html#abi-sys-bus-event-source-devices-hisi-ptt-sicl-id-core-id-tune-qos-tx-np)
* [/sys/bus/event\_source/devices/hisi\_ptt<sicl\_id>\_<core\_id>/tune/qos\_tx\_p](abi-testing.html#abi-sys-bus-event-source-devices-hisi-ptt-sicl-id-core-id-tune-qos-tx-p)
* [/sys/bus/event\_source/devices/hisi\_ptt<sicl\_id>\_<core\_id>/tune/rx\_alloc\_buf\_level](abi-testing.html#abi-sys-bus-event-source-devices-hisi-ptt-sicl-id-core-id-tune-rx-alloc-buf-level)
* [/sys/bus/event\_source/devices/hisi\_ptt<sicl\_id>\_<core\_id>/tune/tx\_alloc\_buf\_level](abi-testing.html#abi-sys-bus-event-source-devices-hisi-ptt-sicl-id-core-id-tune-tx-alloc-buf-level)
* [/sys/devices/hisi\_ptt<sicl\_id>\_<core\_id>/root\_port\_filters](abi-testing.html#abi-sys-devices-hisi-ptt-sicl-id-core-id-root-port-filters)
* [/sys/devices/hisi\_ptt<sicl\_id>\_<core\_id>/root\_port\_filters/multiselect](abi-testing.html#abi-sys-devices-hisi-ptt-sicl-id-core-id-root-port-filters-multiselect)
* [/sys/devices/hisi\_ptt<sicl\_id>\_<core\_id>/root\_port\_filters/<bdf>](abi-testing.html#abi-sys-devices-hisi-ptt-sicl-id-core-id-root-port-filters-bdf)
* [/sys/devices/hisi\_ptt<sicl\_id>\_<core\_id>/requester\_filters](abi-testing.html#abi-sys-devices-hisi-ptt-sicl-id-core-id-requester-filters)
* [/sys/devices/hisi\_ptt<sicl\_id>\_<core\_id>/requester\_filters/multiselect](abi-testing.html#abi-sys-devices-hisi-ptt-sicl-id-core-id-requester-filters-multiselect)
* [/sys/devices/hisi\_ptt<sicl\_id>\_<core\_id>/requester\_filters/<bdf>](abi-testing.html#abi-sys-devices-hisi-ptt-sicl-id-core-id-requester-filters-bdf)

## ABI file testing/sysfs-bus-event\_source-devices-hv\_24x7

Has the following ABI:

* [/sys/bus/event\_source/devices/hv\_24x7/format](abi-testing.html#abi-sys-bus-event-source-devices-hv-24x7-format)
* [/sys/bus/event\_source/devices/hv\_24x7/interface/catalog](abi-testing.html#abi-sys-bus-event-source-devices-hv-24x7-interface-catalog)
* [/sys/bus/event\_source/devices/hv\_24x7/interface/catalog\_length](abi-testing.html#abi-sys-bus-event-source-devices-hv-24x7-interface-catalog-length)
* [/sys/bus/event\_source/devices/hv\_24x7/interface/catalog\_version](abi-testing.html#abi-sys-bus-event-source-devices-hv-24x7-interface-catalog-version)
* [/sys/devices/hv\_24x7/interface/sockets](abi-testing.html#abi-sys-devices-hv-24x7-interface-sockets)
* [/sys/devices/hv\_24x7/interface/chipspersocket](abi-testing.html#abi-sys-devices-hv-24x7-interface-chipspersocket)
* [/sys/devices/hv\_24x7/interface/coresperchip](abi-testing.html#abi-sys-devices-hv-24x7-interface-coresperchip)
* [/sys/devices/hv\_24x7/cpumask](abi-testing.html#abi-sys-devices-hv-24x7-cpumask)
* [/sys/bus/event\_source/devices/hv\_24x7/event\_descs/<event-name>](abi-testing.html#abi-sys-bus-event-source-devices-hv-24x7-event-descs-event-name)
* [/sys/bus/event\_source/devices/hv\_24x7/event\_long\_descs/<event-name>](abi-testing.html#abi-sys-bus-event-source-devices-hv-24x7-event-long-descs-event-name)

## ABI file testing/sysfs-bus-event\_source-devices-hv\_gpci

Has the following ABI:

* [/sys/bus/event\_source/devices/hv\_gpci/format](abi-testing.html#abi-sys-bus-event-source-devices-hv-gpci-format)
* [/sys/bus/event\_source/devices/hv\_gpci/interface/collect\_privileged](abi-testing.html#abi-sys-bus-event-source-devices-hv-gpci-interface-collect-privileged)
* [/sys/bus/event\_source/devices/hv\_gpci/interface/ga](abi-testing.html#abi-sys-bus-event-source-devices-hv-gpci-interface-ga)
* [/sys/bus/event\_source/devices/hv\_gpci/interface/expanded](abi-testing.html#abi-sys-bus-event-source-devices-hv-gpci-interface-expanded)
* [/sys/bus/event\_source/devices/hv\_gpci/interface/lab](abi-testing.html#abi-sys-bus-event-source-devices-hv-gpci-interface-lab)
* [/sys/bus/event\_source/devices/hv\_gpci/interface/version](abi-testing.html#abi-sys-bus-event-source-devices-hv-gpci-interface-version)
* [/sys/bus/event\_source/devices/hv\_gpci/interface/kernel\_version](abi-testing.html#abi-sys-bus-event-source-devices-hv-gpci-interface-kernel-version)
* [/sys/devices/hv\_gpci/cpumask](abi-testing.html#abi-sys-devices-hv-gpci-cpumask)
* [/sys/devices/hv\_gpci/interface/processor\_bus\_topology](abi-testing.html#abi-sys-devices-hv-gpci-interface-processor-bus-topology)
* [/sys/devices/hv\_gpci/interface/processor\_config](abi-testing.html#abi-sys-devices-hv-gpci-interface-processor-config)
* [/sys/devices/hv\_gpci/interface/affinity\_domain\_via\_virtual\_processor](abi-testing.html#abi-sys-devices-hv-gpci-interface-affinity-domain-via-virtual-processor)
* [/sys/devices/hv\_gpci/interface/affinity\_domain\_via\_domain](abi-testing.html#abi-sys-devices-hv-gpci-interface-affinity-domain-via-domain)
* [/sys/devices/hv\_gpci/interface/affinity\_domain\_via\_partition](abi-testing.html#abi-sys-devices-hv-gpci-interface-affinity-domain-via-partition)

## ABI file testing/sysfs-bus-event\_source-devices-iommu

Has the following ABI:

* [/sys/bus/event\_source/devices/dmar\*/format](abi-testing.html#abi-sys-bus-event-source-devices-dmar-format)
* [/sys/bus/event\_source/devices/dmar\*/cpumask](abi-testing.html#abi-sys-bus-event-source-devices-dmar-cpumask)

## ABI file testing/sysfs-bus-event\_source-devices-rdpmc

Has the following ABI:

* [/sys/bus/event\_source/devices/cpu.../rdpmc](abi-testing.html#abi-sys-bus-event-source-devices-cpu-rdpmc)

## ABI file testing/sysfs-bus-event\_source-devices-uncore

Has the following ABI:

* [/sys/bus/event\_source/devices/uncore\_\*/alias](abi-testing.html#abi-sys-bus-event-source-devices-uncore-alias)

## ABI file testing/sysfs-bus-event\_source-devices-vpa-dtl

Has the following ABI:

* [/sys/bus/event\_source/devices/vpa\_dtl/format](abi-testing.html#abi-sys-bus-event-source-devices-vpa-dtl-format)
* [/sys/bus/event\_source/devices/vpa\_dtl/events](abi-testing.html#abi-sys-bus-event-source-devices-vpa-dtl-events)

## ABI file testing/sysfs-bus-event\_source-devices-vpa-pmu

Has the following ABI:

* [/sys/bus/event\_source/devices/vpa\_pmu/format](abi-testing.html#abi-sys-bus-event-source-devices-vpa-pmu-format)
* [/sys/bus/event\_source/devices/vpa\_pmu/events](abi-testing.html#abi-sys-bus-event-source-devices-vpa-pmu-events)

## ABI file testing/sysfs-bus-fcoe

Has the following ABI:

* [/sys/bus/fcoe/](abi-testing.html#abi-sys-bus-fcoe)
* [/sys/bus/fcoe/devices/ctlr\_X](abi-testing.html#abi-sys-bus-fcoe-devices-ctlr-x)
* [/sys/bus/fcoe/devices/fcf\_X](abi-testing.html#abi-sys-bus-fcoe-devices-fcf-x)

## ABI file testing/sysfs-bus-fsi

Has the following ABI:

* [/sys/bus/platform/devices/../fsi-master/fsi0/rescan](abi-testing.html#abi-sys-bus-platform-devices-fsi-master-fsi0-rescan)
* [/sys/bus/platform/devices/../fsi-master/fsi0/break](abi-testing.html#abi-sys-bus-platform-devices-fsi-master-fsi0-break)
* [/sys/bus/platform/devices/../fsi-master/fsi0/slave@00:00/term](abi-testing.html#abi-sys-bus-platform-devices-fsi-master-fsi0-slave-00-00-term)
* [/sys/bus/platform/devices/../fsi-master/fsi0/slave@00:00/raw](abi-testing.html#abi-sys-bus-platform-devices-fsi-master-fsi0-slave-00-00-raw)
* [/sys/bus/platform/devices/../cfam\_reset](abi-testing.html#abi-sys-bus-platform-devices-cfam-reset)

## ABI file testing/sysfs-bus-fsi-devices-sbefifo

Has the following ABI:

* [/sys/bus/fsi/devices/XX.XX.00:06/sbefifoX/timeout](abi-testing.html#abi-sys-bus-fsi-devices-xx-xx-00-06-sbefifox-timeout)

## ABI file testing/sysfs-bus-fsl-mc

Has the following ABI:

* [/sys/bus/fsl-mc/drivers/.../bind](abi-testing.html#abi-sys-bus-fsl-mc-drivers-bind)
* [/sys/bus/fsl-mc/drivers/.../unbind](abi-testing.html#abi-sys-bus-fsl-mc-drivers-unbind)

## ABI file testing/sysfs-bus-hsi

Has the following ABI:

* [/sys/bus/hsi](abi-testing.html#abi-sys-bus-hsi)
* [/sys/bus/hsi/devices/.../modalias](abi-testing.html#abi-sys-bus-hsi-devices-modalias)

## ABI file testing/sysfs-bus-i2c-devices-bq32k

Has the following ABI:

* [/sys/bus/i2c/devices/.../trickle\_charge\_bypass](abi-testing.html#abi-sys-bus-i2c-devices-trickle-charge-bypass)

## ABI file testing/sysfs-bus-i2c-devices-fsa9480

Has the following ABI:

* [/sys/bus/i2c/devices/.../device](abi-testing.html#abi-sys-bus-i2c-devices-device)
* [/sys/bus/i2c/devices/.../switch](abi-testing.html#abi-sys-bus-i2c-devices-switch)

## ABI file testing/sysfs-bus-i2c-devices-hm6352

Has the following ABI:

* [/sys/bus/i2c/devices/.../heading0\_input](abi-testing.html#abi-sys-bus-i2c-devices-heading0-input)
* [/sys/bus/i2c/devices/.../power\_state](abi-testing.html#abi-sys-bus-i2c-devices-power-state)
* [/sys/bus/i2c/devices/.../calibration](abi-testing.html#abi-sys-bus-i2c-devices-calibration)

## ABI file testing/sysfs-bus-i2c-devices-lm3533

Has the following ABI:

* [/sys/bus/i2c/devices/.../output\_hvled[n]](abi-testing.html#abi-sys-bus-i2c-devices-output-hvled-n)
* [/sys/bus/i2c/devices/.../output\_lvled[n]](abi-testing.html#abi-sys-bus-i2c-devices-output-lvled-n)

## ABI file testing/sysfs-bus-i2c-devices-m24lr

Has the following ABI:

* [/sys/bus/i2c/devices/<busnum>-<primary-addr>/unlock](abi-testing.html#abi-sys-bus-i2c-devices-busnum-primary-addr-unlock)
* [/sys/bus/i2c/devices/<busnum>-<primary-addr>/new\_pass](abi-testing.html#abi-sys-bus-i2c-devices-busnum-primary-addr-new-pass)
* [/sys/bus/i2c/devices/<busnum>-<primary-addr>/uid](abi-testing.html#abi-sys-bus-i2c-devices-busnum-primary-addr-uid)
* [/sys/bus/i2c/devices/<busnum>-<primary-addr>/total\_sectors](abi-testing.html#abi-sys-bus-i2c-devices-busnum-primary-addr-total-sectors)
* [/sys/bus/i2c/devices/<busnum>-<primary-addr>/sss](abi-testing.html#abi-sys-bus-i2c-devices-busnum-primary-addr-sss)

## ABI file testing/sysfs-bus-i2c-devices-pca954x

Has the following ABI:

* [/sys/bus/i2c/.../idle\_state](abi-testing.html#abi-sys-bus-i2c-idle-state)

## ABI file testing/sysfs-bus-i2c-devices-turris-omnia-mcu

Has the following ABI:

* [/sys/bus/i2c/devices/<mcu\_device>/board\_revision](abi-testing.html#abi-sys-bus-i2c-devices-mcu-device-board-revision)
* [/sys/bus/i2c/devices/<mcu\_device>/first\_mac\_address](abi-testing.html#abi-sys-bus-i2c-devices-mcu-device-first-mac-address)
* [/sys/bus/i2c/devices/<mcu\_device>/front\_button\_mode](abi-testing.html#abi-sys-bus-i2c-devices-mcu-device-front-button-mode)
* [/sys/bus/i2c/devices/<mcu\_device>/front\_button\_poweron](abi-testing.html#abi-sys-bus-i2c-devices-mcu-device-front-button-poweron)
* [/sys/bus/i2c/devices/<mcu\_device>/fw\_features](abi-testing.html#abi-sys-bus-i2c-devices-mcu-device-fw-features)
* [/sys/bus/i2c/devices/<mcu\_device>/fw\_version\_hash\_application](abi-testing.html#abi-sys-bus-i2c-devices-mcu-device-fw-version-hash-application)
* [/sys/bus/i2c/devices/<mcu\_device>/fw\_version\_hash\_bootloader](abi-testing.html#abi-sys-bus-i2c-devices-mcu-device-fw-version-hash-bootloader)
* [/sys/bus/i2c/devices/<mcu\_device>/mcu\_type](abi-testing.html#abi-sys-bus-i2c-devices-mcu-device-mcu-type)
* [/sys/bus/i2c/devices/<mcu\_device>/reset\_selector](abi-testing.html#abi-sys-bus-i2c-devices-mcu-device-reset-selector)
* [/sys/bus/i2c/devices/<mcu\_device>/serial\_number](abi-testing.html#abi-sys-bus-i2c-devices-mcu-device-serial-number)

## ABI file testing/sysfs-bus-i3c

Has the following ABI:

* [/sys/bus/i3c/devices/i3c-<bus-id>](abi-testing.html#abi-sys-bus-i3c-devices-i3c-bus-id)
* [/sys/bus/i3c/devices/i3c-<bus-id>/current\_master](abi-testing.html#abi-sys-bus-i3c-devices-i3c-bus-id-current-master)
* [/sys/bus/i3c/devices/i3c-<bus-id>/mode](abi-testing.html#abi-sys-bus-i3c-devices-i3c-bus-id-mode)
* [/sys/bus/i3c/devices/i3c-<bus-id>/i3c\_scl\_frequency](abi-testing.html#abi-sys-bus-i3c-devices-i3c-bus-id-i3c-scl-frequency)
* [/sys/bus/i3c/devices/i3c-<bus-id>/i2c\_scl\_frequency](abi-testing.html#abi-sys-bus-i3c-devices-i3c-bus-id-i2c-scl-frequency)
* [/sys/bus/i3c/devices/i3c-<bus-id>/dynamic\_address](abi-testing.html#abi-sys-bus-i3c-devices-i3c-bus-id-dynamic-address)
* [/sys/bus/i3c/devices/i3c-<bus-id>/bcr](abi-testing.html#abi-sys-bus-i3c-devices-i3c-bus-id-bcr)
* [/sys/bus/i3c/devices/i3c-<bus-id>/dcr](abi-testing.html#abi-sys-bus-i3c-devices-i3c-bus-id-dcr)
* [/sys/bus/i3c/devices/i3c-<bus-id>/pid](abi-testing.html#abi-sys-bus-i3c-devices-i3c-bus-id-pid)
* [/sys/bus/i3c/devices/i3c-<bus-id>/hdrcap](abi-testing.html#abi-sys-bus-i3c-devices-i3c-bus-id-hdrcap)
* [/sys/bus/i3c/devices/i3c-<bus-id>/hotjoin](abi-testing.html#abi-sys-bus-i3c-devices-i3c-bus-id-hotjoin)
* [/sys/bus/i3c/devices/i3c-<bus-id>/<bus-id>-<device-pid>](abi-testing.html#abi-sys-bus-i3c-devices-i3c-bus-id-bus-id-device-pid)
* [/sys/bus/i3c/devices/i3c-<bus-id>/<bus-id>-<device-pid>/dynamic\_address](abi-testing.html#abi-sys-bus-i3c-devices-i3c-bus-id-bus-id-device-pid-dynamic-address)
* [/sys/bus/i3c/devices/i3c-<bus-id>/<bus-id>-<device-pid>/bcr](abi-testing.html#abi-sys-bus-i3c-devices-i3c-bus-id-bus-id-device-pid-bcr)
* [/sys/bus/i3c/devices/i3c-<bus-id>/<bus-id>-<device-pid>/dcr](abi-testing.html#abi-sys-bus-i3c-devices-i3c-bus-id-bus-id-device-pid-dcr)
* [/sys/bus/i3c/devices/i3c-<bus-id>/<bus-id>-<device-pid>/pid](abi-testing.html#abi-sys-bus-i3c-devices-i3c-bus-id-bus-id-device-pid-pid)
* [/sys/bus/i3c/devices/i3c-<bus-id>/<bus-id>-<device-pid>/hdrcap](abi-testing.html#abi-sys-bus-i3c-devices-i3c-bus-id-bus-id-device-pid-hdrcap)
* [/sys/bus/i3c/devices/<bus-id>-<device-pid>](abi-testing.html#abi-sys-bus-i3c-devices-bus-id-device-pid)
* [/sys/bus/i3c/devices/i3c-<bus-id>/<bus-id>-<device-pid>/dev\_nack\_retry\_count](abi-testing.html#abi-sys-bus-i3c-devices-i3c-bus-id-bus-id-device-pid-dev-nack-retry-count)
* [/sys/bus/i3c/devices/i3c-<bus-id>/do\_daa](abi-testing.html#abi-sys-bus-i3c-devices-i3c-bus-id-do-daa)

## ABI file testing/sysfs-bus-iio

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex)
* [/sys/bus/iio/devices/triggerX](abi-testing.html#abi-sys-bus-iio-devices-triggerx)
* [/sys/bus/iio/devices/iio:deviceX/buffer](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-buffer)
* [/sys/bus/iio/devices/iio:deviceX/name](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-name)
* [/sys/bus/iio/devices/iio:deviceX/label](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-label)
* [/sys/bus/iio/devices/iio:deviceX/current\_timestamp\_clock](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-current-timestamp-clock)
* [/sys/bus/iio/devices/iio:deviceX/sampling\_frequency](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-sampling-frequency)
* [/sys/bus/iio/devices/iio:deviceX/in\_intensity\_sampling\_frequency](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-sampling-frequency)
* [/sys/bus/iio/devices/iio:deviceX/buffer/sampling\_frequency](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-sampling-frequency)
* [/sys/bus/iio/devices/iio:deviceX/events/sampling\_frequency](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-sampling-frequency)
* [/sys/bus/iio/devices/triggerX/sampling\_frequency](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-sampling-frequency)
* [/sys/bus/iio/devices/iio:deviceX/sampling\_frequency\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-sampling-frequency-available)
* [/sys/bus/iio/devices/iio:deviceX/in\_intensity\_sampling\_frequency\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-sampling-frequency-available)
* [/sys/bus/iio/devices/iio:deviceX/in\_proximity\_sampling\_frequency\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-sampling-frequency-available)
* [/sys/.../iio:deviceX/buffer/sampling\_frequency\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-sampling-frequency-available)
* [/sys/bus/iio/devices/triggerX/sampling\_frequency\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-sampling-frequency-available)
* [/sys/bus/iio/devices/iio:deviceX/oversampling\_ratio](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-oversampling-ratio)
* [/sys/bus/iio/devices/iio:deviceX/oversampling\_ratio\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-oversampling-ratio-available)
* [/sys/bus/iio/devices/iio:deviceX/in\_voltageY\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_voltageY\_supply\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_voltageY-voltageZ\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-voltagez-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_altvoltageY\_rms\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-altvoltagey-rms-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_powerY\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-powery-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_powerY\_active\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-powery-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_powerY\_reactive\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-powery-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_powerY\_apparent\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-powery-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_powerY\_powerfactor](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-powery-powerfactor)
* [/sys/bus/iio/devices/iio:deviceX/in\_capacitanceY\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-capacitancey-raw)
* [/sys/.../iio:deviceX/in\_capacitanceY-capacitanceZ\_raw](abi-testing.html#abi-sys-iio-devicex-in-capacitancey-capacitancez-raw)
* [/sys/.../iio:deviceX/in\_capacitanceY-capacitanceZ\_zeropoint](abi-testing.html#abi-sys-iio-devicex-in-capacitancey-capacitancez-zeropoint)
* [/sys/bus/iio/devices/iio:deviceX/in\_temp\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-temp-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_tempY\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-temp-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_temp\_x\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-temp-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_temp\_y\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-temp-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_temp\_ambient\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-temp-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_temp\_object\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-temp-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_tempY\_input](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-tempy-input)
* [/sys/bus/iio/devices/iio:deviceX/in\_temp\_input](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-tempy-input)
* [/sys/bus/iio/devices/iio:deviceX/in\_accel\_x\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_accel\_y\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_accel\_z\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_accel\_linear\_x\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-linear-x-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_accel\_linear\_y\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-linear-x-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_accel\_linear\_z\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-linear-x-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_gravity\_x\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-gravity-x-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_gravity\_y\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-gravity-x-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_gravity\_z\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-gravity-x-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_deltaangl\_x\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-deltaangl-x-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_deltaangl\_y\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-deltaangl-x-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_deltaangl\_z\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-deltaangl-x-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_deltavelocity\_x\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-deltavelocity-x-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_deltavelocity\_y\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-deltavelocity-x-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_deltavelocity\_z\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-deltavelocity-x-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_angl\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-angl-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_anglY\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-angl-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_positionrelative\_x\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-positionrelative-x-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_positionrelative\_y\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-positionrelative-x-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_anglvel\_x\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-anglvel-x-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_anglvel\_y\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-anglvel-x-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_anglvel\_z\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-anglvel-x-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_incli\_x\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-incli-x-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_incli\_y\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-incli-x-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_incli\_z\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-incli-x-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_magn\_x\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-magn-x-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_magn\_y\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-magn-x-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_magn\_z\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-magn-x-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_accel\_x\_peak\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-peak-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_accel\_y\_peak\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-peak-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_accel\_z\_peak\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-peak-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_humidityrelative\_peak\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-peak-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_temp\_peak\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-peak-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_humidityrelative\_trough\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-humidityrelative-trough-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_temp\_trough\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-humidityrelative-trough-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_accel\_xyz\_squared\_peak\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-xyz-squared-peak-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_pressureY\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-pressurey-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_pressure\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-pressurey-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_pressureY\_input](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-pressurey-input)
* [/sys/bus/iio/devices/iio:deviceX/in\_pressure\_input](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-pressurey-input)
* [/sys/bus/iio/devices/iio:deviceX/in\_humidityrelative\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-humidityrelative-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_humidityrelative\_input](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-humidityrelative-input)
* [/sys/bus/iio/devices/iio:deviceX/in\_Y\_mean\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-y-mean-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_accel\_offset](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-offset)
* [/sys/bus/iio/devices/iio:deviceX/in\_accel\_x\_offset](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-offset)
* [/sys/bus/iio/devices/iio:deviceX/in\_accel\_y\_offset](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-offset)
* [/sys/bus/iio/devices/iio:deviceX/in\_accel\_z\_offset](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-offset)
* [/sys/bus/iio/devices/iio:deviceX/in\_altvoltage\_q\_offset](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-offset)
* [/sys/bus/iio/devices/iio:deviceX/in\_altvoltage\_i\_offset](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-offset)
* [/sys/bus/iio/devices/iio:deviceX/in\_voltageY\_offset](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-offset)
* [/sys/bus/iio/devices/iio:deviceX/in\_voltage\_offset](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-offset)
* [/sys/bus/iio/devices/iio:deviceX/in\_voltageY\_i\_offset](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-offset)
* [/sys/bus/iio/devices/iio:deviceX/in\_voltageY\_q\_offset](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-offset)
* [/sys/bus/iio/devices/iio:deviceX/in\_currentY\_offset](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-offset)
* [/sys/bus/iio/devices/iio:deviceX/in\_current\_offset](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-offset)
* [/sys/bus/iio/devices/iio:deviceX/in\_tempY\_offset](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-offset)
* [/sys/bus/iio/devices/iio:deviceX/in\_temp\_offset](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-offset)
* [/sys/bus/iio/devices/iio:deviceX/in\_pressureY\_offset](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-offset)
* [/sys/bus/iio/devices/iio:deviceX/in\_pressure\_offset](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-offset)
* [/sys/bus/iio/devices/iio:deviceX/in\_humidityrelative\_offset](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-offset)
* [/sys/bus/iio/devices/iio:deviceX/in\_magn\_offset](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-offset)
* [/sys/bus/iio/devices/iio:deviceX/in\_rot\_offset](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-offset)
* [/sys/bus/iio/devices/iio:deviceX/in\_angl\_offset](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-offset)
* [/sys/bus/iio/devices/iio:deviceX/in\_capacitanceY\_offset](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-offset)
* [/sys/bus/iio/devices/iio:deviceX/in\_voltageY\_scale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-scale)
* [/sys/bus/iio/devices/iio:deviceX/in\_voltageY\_q\_scale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-scale)
* [/sys/bus/iio/devices/iio:deviceX/in\_voltageY\_supply\_scale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-scale)
* [/sys/bus/iio/devices/iio:deviceX/in\_voltage\_scale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-scale)
* [/sys/bus/iio/devices/iio:deviceX/in\_voltage-voltage\_scale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-scale)
* [/sys/bus/iio/devices/iio:deviceX/out\_voltageY\_scale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-scale)
* [/sys/bus/iio/devices/iio:deviceX/out\_altvoltageY\_scale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-scale)
* [/sys/bus/iio/devices/iio:deviceX/in\_currentY\_scale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-scale)
* [/sys/bus/iio/devices/iio:deviceX/in\_currentY\_supply\_scale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-scale)
* [/sys/bus/iio/devices/iio:deviceX/in\_current\_scale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-scale)
* [/sys/bus/iio/devices/iio:deviceX/in\_current\_q\_scale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-scale)
* [/sys/bus/iio/devices/iio:deviceX/in\_accel\_scale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-scale)
* [/sys/bus/iio/devices/iio:deviceX/in\_accel\_peak\_scale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-scale)
* [/sys/bus/iio/devices/iio:deviceX/in\_anglvel\_scale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-scale)
* [/sys/bus/iio/devices/iio:deviceX/in\_energy\_scale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-scale)
* [/sys/bus/iio/devices/iio:deviceX/in\_distance\_scale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-scale)
* [/sys/bus/iio/devices/iio:deviceX/in\_magn\_scale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-scale)
* [/sys/bus/iio/devices/iio:deviceX/in\_magn\_x\_scale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-scale)
* [/sys/bus/iio/devices/iio:deviceX/in\_magn\_y\_scale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-scale)
* [/sys/bus/iio/devices/iio:deviceX/in\_magn\_z\_scale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-scale)
* [/sys/bus/iio/devices/iio:deviceX/in\_rot\_from\_north\_magnetic\_scale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-scale)
* [/sys/bus/iio/devices/iio:deviceX/in\_rot\_from\_north\_true\_scale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-scale)
* [/sys/bus/iio/devices/iio:deviceX/in\_rot\_from\_north\_magnetic\_tilt\_comp\_scale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-scale)
* [/sys/bus/iio/devices/iio:deviceX/in\_rot\_from\_north\_true\_tilt\_comp\_scale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-scale)
* [/sys/bus/iio/devices/iio:deviceX/in\_pressureY\_scale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-scale)
* [/sys/bus/iio/devices/iio:deviceX/in\_pressure\_scale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-scale)
* [/sys/bus/iio/devices/iio:deviceX/in\_humidityrelative\_scale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-scale)
* [/sys/bus/iio/devices/iio:deviceX/in\_velocity\_sqrt(x^2+y^2+z^2)\_scale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-scale)
* [/sys/bus/iio/devices/iio:deviceX/in\_illuminance\_scale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-scale)
* [/sys/bus/iio/devices/iio:deviceX/in\_countY\_scale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-scale)
* [/sys/bus/iio/devices/iio:deviceX/in\_deltaangl\_scale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-scale)
* [/sys/bus/iio/devices/iio:deviceX/in\_deltavelocity\_scale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-scale)
* [/sys/bus/iio/devices/iio:deviceX/in\_angl\_scale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-scale)
* [/sys/bus/iio/devices/iio:deviceX/in\_intensity\_x\_scale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-scale)
* [/sys/bus/iio/devices/iio:deviceX/in\_intensity\_y\_scale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-scale)
* [/sys/bus/iio/devices/iio:deviceX/in\_intensity\_z\_scale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-scale)
* [/sys/bus/iio/devices/iio:deviceX/in\_intensity\_red\_scale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-scale)
* [/sys/bus/iio/devices/iio:deviceX/in\_intensity\_green\_scale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-scale)
* [/sys/bus/iio/devices/iio:deviceX/in\_intensity\_blue\_scale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-scale)
* [/sys/bus/iio/devices/iio:deviceX/in\_concentration\_co2\_scale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-scale)
* [/sys/bus/iio/devices/iio:deviceX/in\_accel\_x\_calibbias](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibbias)
* [/sys/bus/iio/devices/iio:deviceX/in\_accel\_y\_calibbias](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibbias)
* [/sys/bus/iio/devices/iio:deviceX/in\_accel\_z\_calibbias](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibbias)
* [/sys/bus/iio/devices/iio:deviceX/in\_altvoltageY\_i\_calibbias](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibbias)
* [/sys/bus/iio/devices/iio:deviceX/in\_altvoltageY\_q\_calibbias](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibbias)
* [/sys/bus/iio/devices/iio:deviceX/in\_anglvel\_x\_calibbias](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibbias)
* [/sys/bus/iio/devices/iio:deviceX/in\_anglvel\_y\_calibbias](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibbias)
* [/sys/bus/iio/devices/iio:deviceX/in\_anglvel\_z\_calibbias](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibbias)
* [/sys/bus/iio/devices/iio:deviceX/in\_capacitance\_calibbias](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibbias)
* [/sys/bus/iio/devices/iio:deviceX/in\_illuminance\_calibbias](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibbias)
* [/sys/bus/iio/devices/iio:deviceX/in\_illuminance0\_calibbias](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibbias)
* [/sys/bus/iio/devices/iio:deviceX/in\_intensityY\_calibbias](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibbias)
* [/sys/bus/iio/devices/iio:deviceX/in\_magn\_x\_calibbias](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibbias)
* [/sys/bus/iio/devices/iio:deviceX/in\_magn\_y\_calibbias](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibbias)
* [/sys/bus/iio/devices/iio:deviceX/in\_magn\_z\_calibbias](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibbias)
* [/sys/bus/iio/devices/iio:deviceX/in\_pressure\_calibbias](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibbias)
* [/sys/bus/iio/devices/iio:deviceX/in\_pressureY\_calibbias](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibbias)
* [/sys/bus/iio/devices/iio:deviceX/in\_proximity\_calibbias](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibbias)
* [/sys/bus/iio/devices/iio:deviceX/in\_proximity0\_calibbias](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibbias)
* [/sys/bus/iio/devices/iio:deviceX/in\_resistance\_calibbias](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibbias)
* [/sys/bus/iio/devices/iio:deviceX/in\_temp\_calibbias](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibbias)
* [/sys/bus/iio/devices/iio:deviceX/in\_voltageY\_calibbias](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibbias)
* [/sys/bus/iio/devices/iio:deviceX/out\_currentY\_calibbias](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibbias)
* [/sys/bus/iio/devices/iio:deviceX/out\_voltageY\_calibbias](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibbias)
* [/sys/bus/iio/devices/iio:deviceX/in\_accel\_calibbias\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-calibbias-available)
* [/sys/bus/iio/devices/iio:deviceX/in\_anglvel\_calibbias\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-calibbias-available)
* [/sys/bus/iio/devices/iio:deviceX/in\_temp\_calibbias\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-calibbias-available)
* [/sys/bus/iio/devices/iio:deviceX/in\_proximity\_calibbias\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-calibbias-available)
* [/sys/bus/iio/devices/iio:deviceX/in\_voltageY\_calibbias\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-calibbias-available)
* [/sys/bus/iio/devices/iio:deviceX/out\_voltageY\_calibbias\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-calibbias-available)
* [/sys/bus/iio/devices/iio:deviceX/in\_voltageY\_convdelay](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-convdelay)
* [/sys/bus/iio/devices/iio:deviceX/in\_voltageY\_convdelay\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-convdelay-available)
* [/sys/bus/iio/devices/iio:deviceX/in\_accel\_x\_calibscale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibscale)
* [/sys/bus/iio/devices/iio:deviceX/in\_accel\_y\_calibscale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibscale)
* [/sys/bus/iio/devices/iio:deviceX/in\_accel\_z\_calibscale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibscale)
* [/sys/bus/iio/devices/iio:deviceX/in\_altvoltage\_calibscale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibscale)
* [/sys/bus/iio/devices/iio:deviceX/in\_anglvel\_x\_calibscale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibscale)
* [/sys/bus/iio/devices/iio:deviceX/in\_anglvel\_y\_calibscale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibscale)
* [/sys/bus/iio/devices/iio:deviceX/in\_anglvel\_z\_calibscale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibscale)
* [/sys/bus/iio/devices/iio:deviceX/in\_capacitance\_calibscale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibscale)
* [/sys/bus/iio/devices/iio:deviceX/in\_illuminance\_calibscale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibscale)
* [/sys/bus/iio/devices/iio:deviceX/in\_illuminance0\_calibscale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibscale)
* [/sys/bus/iio/devices/iio:deviceX/in\_intensity\_both\_calibscale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibscale)
* [/sys/bus/iio/devices/iio:deviceX/in\_intensity\_calibscale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibscale)
* [/sys/bus/iio/devices/iio:deviceX/in\_intensity\_ir\_calibscale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibscale)
* [/sys/bus/iio/devices/iio:deviceX/in\_magn\_x\_calibscale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibscale)
* [/sys/bus/iio/devices/iio:deviceX/in\_magn\_y\_calibscale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibscale)
* [/sys/bus/iio/devices/iio:deviceX/in\_magn\_z\_calibscale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibscale)
* [/sys/bus/iio/devices/iio:deviceX/in\_pressure\_calibscale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibscale)
* [/sys/bus/iio/devices/iio:deviceX/in\_pressureY\_calibscale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibscale)
* [/sys/bus/iio/devices/iio:deviceX/in\_proximity0\_calibscale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibscale)
* [/sys/bus/iio/devices/iio:deviceX/in\_voltage\_calibscale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibscale)
* [/sys/bus/iio/devices/iio:deviceX/in\_voltageY\_calibscale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibscale)
* [/sys/bus/iio/devices/iio:deviceX/in\_voltageY\_supply\_calibscale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibscale)
* [/sys/bus/iio/devices/iio:deviceX/out\_currentY\_calibscale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibscale)
* [/sys/bus/iio/devices/iio:deviceX/out\_voltageY\_calibscale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-x-calibscale)
* [/sys/bus/iio/devices/iio:deviceX/in\_illuminanceY\_calibscale\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-illuminancey-calibscale-available)
* [/sys/bus/iio/devices/iio:deviceX/in\_intensityY\_calibscale\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-illuminancey-calibscale-available)
* [/sys/bus/iio/devices/iio:deviceX/in\_proximityY\_calibscale\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-illuminancey-calibscale-available)
* [/sys/bus/iio/devices/iio:deviceX/in\_voltageY\_calibscale\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-illuminancey-calibscale-available)
* [/sys/bus/iio/devices/iio:deviceX/in\_activity\_calibgender](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-activity-calibgender)
* [/sys/bus/iio/devices/iio:deviceX/in\_energy\_calibgender](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-activity-calibgender)
* [/sys/bus/iio/devices/iio:deviceX/in\_distance\_calibgender](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-activity-calibgender)
* [/sys/bus/iio/devices/iio:deviceX/in\_velocity\_calibgender](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-activity-calibgender)
* [/sys/bus/iio/devices/iio:deviceX/in\_activity\_calibgender\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-activity-calibgender-available)
* [/sys/bus/iio/devices/iio:deviceX/in\_energy\_calibgender\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-activity-calibgender-available)
* [/sys/bus/iio/devices/iio:deviceX/in\_distance\_calibgender\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-activity-calibgender-available)
* [/sys/bus/iio/devices/iio:deviceX/in\_velocity\_calibgender\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-activity-calibgender-available)
* [/sys/bus/iio/devices/iio:deviceX/in\_activity\_calibheight](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-activity-calibheight)
* [/sys/bus/iio/devices/iio:deviceX/in\_energy\_calibheight](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-activity-calibheight)
* [/sys/bus/iio/devices/iio:deviceX/in\_distance\_calibheight](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-activity-calibheight)
* [/sys/bus/iio/devices/iio:deviceX/in\_velocity\_calibheight](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-activity-calibheight)
* [/sys/bus/iio/devices/iio:deviceX/in\_energy\_calibweight](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-energy-calibweight)
* [/sys/bus/iio/devices/iio:deviceX/in\_accel\_scale\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-scale-available)
* [/sys/.../iio:deviceX/in\_anglvel\_scale\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-scale-available)
* [/sys/.../iio:deviceX/in\_magn\_scale\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-scale-available)
* [/sys/.../iio:deviceX/in\_illuminance\_scale\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-scale-available)
* [/sys/.../iio:deviceX/in\_intensity\_scale\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-scale-available)
* [/sys/.../iio:deviceX/in\_proximity\_scale\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-scale-available)
* [/sys/.../iio:deviceX/in\_voltageY\_scale\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-scale-available)
* [/sys/.../iio:deviceX/in\_voltage-voltage\_scale\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-scale-available)
* [/sys/.../iio:deviceX/out\_voltageY\_scale\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-scale-available)
* [/sys/.../iio:deviceX/out\_altvoltageY\_scale\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-scale-available)
* [/sys/.../iio:deviceX/in\_capacitance\_scale\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-scale-available)
* [/sys/.../iio:deviceX/in\_pressure\_scale\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-scale-available)
* [/sys/.../iio:deviceX/in\_pressureY\_scale\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-scale-available)
* [/sys/bus/iio/devices/iio:deviceX/out\_voltageY\_hardwaregain](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-voltagey-hardwaregain)
* [/sys/bus/iio/devices/iio:deviceX/in\_intensity\_hardwaregain](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-voltagey-hardwaregain)
* [/sys/bus/iio/devices/iio:deviceX/in\_intensity\_red\_hardwaregain](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-voltagey-hardwaregain)
* [/sys/bus/iio/devices/iio:deviceX/in\_intensity\_green\_hardwaregain](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-voltagey-hardwaregain)
* [/sys/bus/iio/devices/iio:deviceX/in\_intensity\_blue\_hardwaregain](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-voltagey-hardwaregain)
* [/sys/bus/iio/devices/iio:deviceX/in\_intensity\_clear\_hardwaregain](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-voltagey-hardwaregain)
* [/sys/bus/iio/devices/iio:deviceX/in\_illuminance\_hardwaregain](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-voltagey-hardwaregain)
* [/sys/bus/iio/devices/iio:deviceX/in\_intensity\_hardwaregain\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-intensity-hardwaregain-available)
* [/sys/.../in\_accel\_filter\_low\_pass\_3db\_frequency](abi-testing.html#abi-sys-in-accel-filter-low-pass-3db-frequency)
* [/sys/.../in\_magn\_filter\_low\_pass\_3db\_frequency](abi-testing.html#abi-sys-in-accel-filter-low-pass-3db-frequency)
* [/sys/.../in\_anglvel\_filter\_low\_pass\_3db\_frequency](abi-testing.html#abi-sys-in-accel-filter-low-pass-3db-frequency)
* [/sys/.../in\_accel\_filter\_high\_pass\_3db\_frequency](abi-testing.html#abi-sys-in-accel-filter-high-pass-3db-frequency)
* [/sys/.../in\_anglvel\_filter\_high\_pass\_3db\_frequency](abi-testing.html#abi-sys-in-accel-filter-high-pass-3db-frequency)
* [/sys/.../in\_magn\_filter\_high\_pass\_3db\_frequency](abi-testing.html#abi-sys-in-accel-filter-high-pass-3db-frequency)
* [/sys/bus/iio/devices/iio:deviceX/out\_voltageY\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-voltagey-raw)
* [/sys/bus/iio/devices/iio:deviceX/out\_altvoltageY\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-voltagey-raw)
* [/sys/bus/iio/devices/iio:deviceX/out\_voltageY&Z\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-voltagey-z-raw)
* [/sys/bus/iio/devices/iio:deviceX/out\_altvoltageY&Z\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-voltagey-z-raw)
* [/sys/bus/iio/devices/iio:deviceX/out\_voltageY\_powerdown\_mode](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-voltagey-powerdown-mode)
* [/sys/bus/iio/devices/iio:deviceX/out\_voltage\_powerdown\_mode](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-voltagey-powerdown-mode)
* [/sys/bus/iio/devices/iio:deviceX/out\_altvoltageY\_powerdown\_mode](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-voltagey-powerdown-mode)
* [/sys/bus/iio/devices/iio:deviceX/out\_altvoltage\_powerdown\_mode](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-voltagey-powerdown-mode)
* [/sys/.../iio:deviceX/out\_voltageY\_powerdown\_mode\_available](abi-testing.html#abi-sys-iio-devicex-out-voltagey-powerdown-mode-available)
* [/sys/.../iio:deviceX/out\_voltage\_powerdown\_mode\_available](abi-testing.html#abi-sys-iio-devicex-out-voltagey-powerdown-mode-available)
* [/sys/.../iio:deviceX/out\_altvoltageY\_powerdown\_mode\_available](abi-testing.html#abi-sys-iio-devicex-out-voltagey-powerdown-mode-available)
* [/sys/.../iio:deviceX/out\_altvoltage\_powerdown\_mode\_available](abi-testing.html#abi-sys-iio-devicex-out-voltagey-powerdown-mode-available)
* [/sys/bus/iio/devices/iio:deviceX/out\_voltageY\_powerdown](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-voltagey-powerdown)
* [/sys/bus/iio/devices/iio:deviceX/out\_voltage\_powerdown](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-voltagey-powerdown)
* [/sys/bus/iio/devices/iio:deviceX/out\_altvoltageY\_powerdown](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-voltagey-powerdown)
* [/sys/bus/iio/devices/iio:deviceX/out\_altvoltage\_powerdown](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-voltagey-powerdown)
* [/sys/bus/iio/devices/iio:deviceX/out\_altvoltageY\_frequency](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-altvoltagey-frequency)
* [/sys/bus/iio/devices/iio:deviceX/in\_altvoltageY\_i\_phase](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-altvoltagey-i-phase)
* [/sys/bus/iio/devices/iio:deviceX/in\_altvoltageY\_q\_phase](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-altvoltagey-i-phase)
* [/sys/bus/iio/devices/iio:deviceX/out\_altvoltageY\_phase](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-altvoltagey-i-phase)
* [/sys/bus/iio/devices/iio:deviceX/out\_altvoltageY\_i\_phase](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-altvoltagey-i-phase)
* [/sys/bus/iio/devices/iio:deviceX/out\_altvoltageY\_q\_phase](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-altvoltagey-i-phase)
* [/sys/bus/iio/devices/iio:deviceX/out\_currentY\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-currenty-raw)
* [/sys/bus/iio/devices/iio:deviceX/events](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-events)
* [/sys/.../iio:deviceX/events/in\_accel\_x\_thresh\_rising\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-thresh-rising-en)
* [/sys/.../iio:deviceX/events/in\_accel\_x\_thresh\_falling\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-thresh-rising-en)
* [/sys/.../iio:deviceX/events/in\_accel\_y\_thresh\_rising\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-thresh-rising-en)
* [/sys/.../iio:deviceX/events/in\_accel\_y\_thresh\_falling\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-thresh-rising-en)
* [/sys/.../iio:deviceX/events/in\_accel\_z\_thresh\_rising\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-thresh-rising-en)
* [/sys/.../iio:deviceX/events/in\_accel\_z\_thresh\_falling\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-thresh-rising-en)
* [/sys/.../iio:deviceX/events/in\_anglvel\_x\_thresh\_rising\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-thresh-rising-en)
* [/sys/.../iio:deviceX/events/in\_anglvel\_x\_thresh\_falling\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-thresh-rising-en)
* [/sys/.../iio:deviceX/events/in\_anglvel\_y\_thresh\_rising\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-thresh-rising-en)
* [/sys/.../iio:deviceX/events/in\_anglvel\_y\_thresh\_falling\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-thresh-rising-en)
* [/sys/.../iio:deviceX/events/in\_anglvel\_z\_thresh\_rising\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-thresh-rising-en)
* [/sys/.../iio:deviceX/events/in\_anglvel\_z\_thresh\_falling\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-thresh-rising-en)
* [/sys/.../iio:deviceX/events/in\_magn\_x\_thresh\_rising\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-thresh-rising-en)
* [/sys/.../iio:deviceX/events/in\_magn\_x\_thresh\_falling\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-thresh-rising-en)
* [/sys/.../iio:deviceX/events/in\_magn\_y\_thresh\_rising\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-thresh-rising-en)
* [/sys/.../iio:deviceX/events/in\_magn\_y\_thresh\_falling\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-thresh-rising-en)
* [/sys/.../iio:deviceX/events/in\_magn\_z\_thresh\_rising\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-thresh-rising-en)
* [/sys/.../iio:deviceX/events/in\_magn\_z\_thresh\_falling\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-thresh-rising-en)
* [/sys/.../iio:deviceX/events/in\_rot\_from\_north\_magnetic\_thresh\_rising\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-thresh-rising-en)
* [/sys/.../iio:deviceX/events/in\_rot\_from\_north\_magnetic\_thresh\_falling\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-thresh-rising-en)
* [/sys/.../iio:deviceX/events/in\_rot\_from\_north\_true\_thresh\_rising\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-thresh-rising-en)
* [/sys/.../iio:deviceX/events/in\_rot\_from\_north\_true\_thresh\_falling\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-thresh-rising-en)
* [/sys/.../iio:deviceX/events/in\_rot\_from\_north\_magnetic\_tilt\_comp\_thresh\_rising\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-thresh-rising-en)
* [/sys/.../iio:deviceX/events/in\_rot\_from\_north\_magnetic\_tilt\_comp\_thresh\_falling\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-thresh-rising-en)
* [/sys/.../iio:deviceX/events/in\_rot\_from\_north\_true\_tilt\_comp\_thresh\_rising\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-thresh-rising-en)
* [/sys/.../iio:deviceX/events/in\_rot\_from\_north\_true\_tilt\_comp\_thresh\_falling\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-thresh-rising-en)
* [/sys/.../iio:deviceX/events/in\_voltageY\_supply\_thresh\_rising\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-thresh-rising-en)
* [/sys/.../iio:deviceX/events/in\_voltageY\_supply\_thresh\_falling\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-thresh-rising-en)
* [/sys/.../iio:deviceX/events/in\_voltageY\_thresh\_rising\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-thresh-rising-en)
* [/sys/.../iio:deviceX/events/in\_voltageY\_thresh\_falling\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-thresh-rising-en)
* [/sys/.../iio:deviceX/events/in\_voltageY\_thresh\_either\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-thresh-rising-en)
* [/sys/.../iio:deviceX/events/in\_tempY\_thresh\_rising\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-thresh-rising-en)
* [/sys/.../iio:deviceX/events/in\_tempY\_thresh\_falling\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-thresh-rising-en)
* [/sys/.../iio:deviceX/events/in\_capacitanceY\_thresh\_rising\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-thresh-rising-en)
* [/sys/.../iio:deviceX/events/in\_capacitanceY\_thresh\_falling\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-thresh-rising-en)
* [/sys/.../iio:deviceX/events/in\_pressure\_thresh\_rising\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-thresh-rising-en)
* [/sys/.../iio:deviceX/events/in\_accel\_x\_roc\_rising\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-roc-rising-en)
* [/sys/.../iio:deviceX/events/in\_accel\_x\_roc\_falling\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-roc-rising-en)
* [/sys/.../iio:deviceX/events/in\_accel\_y\_roc\_rising\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-roc-rising-en)
* [/sys/.../iio:deviceX/events/in\_accel\_y\_roc\_falling\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-roc-rising-en)
* [/sys/.../iio:deviceX/events/in\_accel\_z\_roc\_rising\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-roc-rising-en)
* [/sys/.../iio:deviceX/events/in\_accel\_z\_roc\_falling\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-roc-rising-en)
* [/sys/.../iio:deviceX/events/in\_accel\_x&y&z\_roc\_rising\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-roc-rising-en)
* [/sys/.../iio:deviceX/events/in\_anglvel\_x\_roc\_rising\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-roc-rising-en)
* [/sys/.../iio:deviceX/events/in\_anglvel\_x\_roc\_falling\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-roc-rising-en)
* [/sys/.../iio:deviceX/events/in\_anglvel\_y\_roc\_rising\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-roc-rising-en)
* [/sys/.../iio:deviceX/events/in\_anglvel\_y\_roc\_falling\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-roc-rising-en)
* [/sys/.../iio:deviceX/events/in\_anglvel\_z\_roc\_rising\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-roc-rising-en)
* [/sys/.../iio:deviceX/events/in\_anglvel\_z\_roc\_falling\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-roc-rising-en)
* [/sys/.../iio:deviceX/events/in\_magn\_x\_roc\_rising\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-roc-rising-en)
* [/sys/.../iio:deviceX/events/in\_magn\_x\_roc\_falling\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-roc-rising-en)
* [/sys/.../iio:deviceX/events/in\_magn\_y\_roc\_rising\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-roc-rising-en)
* [/sys/.../iio:deviceX/events/in\_magn\_y\_roc\_falling\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-roc-rising-en)
* [/sys/.../iio:deviceX/events/in\_magn\_z\_roc\_rising\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-roc-rising-en)
* [/sys/.../iio:deviceX/events/in\_magn\_z\_roc\_falling\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-roc-rising-en)
* [/sys/.../iio:deviceX/events/in\_rot\_from\_north\_magnetic\_roc\_rising\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-roc-rising-en)
* [/sys/.../iio:deviceX/events/in\_rot\_from\_north\_magnetic\_roc\_falling\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-roc-rising-en)
* [/sys/.../iio:deviceX/events/in\_rot\_from\_north\_true\_roc\_rising\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-roc-rising-en)
* [/sys/.../iio:deviceX/events/in\_rot\_from\_north\_true\_roc\_falling\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-roc-rising-en)
* [/sys/.../iio:deviceX/events/in\_rot\_from\_north\_magnetic\_tilt\_comp\_roc\_rising\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-roc-rising-en)
* [/sys/.../iio:deviceX/events/in\_rot\_from\_north\_magnetic\_tilt\_comp\_roc\_falling\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-roc-rising-en)
* [/sys/.../iio:deviceX/events/in\_rot\_from\_north\_true\_tilt\_comp\_roc\_rising\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-roc-rising-en)
* [/sys/.../iio:deviceX/events/in\_rot\_from\_north\_true\_tilt\_comp\_roc\_falling\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-roc-rising-en)
* [/sys/.../iio:deviceX/events/in\_voltageY\_supply\_roc\_rising\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-roc-rising-en)
* [/sys/.../iio:deviceX/events/in\_voltageY\_supply\_roc\_falling\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-roc-rising-en)
* [/sys/.../iio:deviceX/events/in\_voltageY\_roc\_rising\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-roc-rising-en)
* [/sys/.../iio:deviceX/events/in\_voltageY\_roc\_falling\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-roc-rising-en)
* [/sys/.../iio:deviceX/events/in\_tempY\_roc\_rising\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-roc-rising-en)
* [/sys/.../iio:deviceX/events/in\_tempY\_roc\_falling\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-roc-rising-en)
* [/sys/.../events/in\_capacitanceY\_adaptive\_thresh\_rising\_en](abi-testing.html#abi-sys-events-in-capacitancey-adaptive-thresh-rising-en)
* [/sys/.../events/in\_capacitanceY\_adaptive\_thresh\_falling\_en](abi-testing.html#abi-sys-events-in-capacitancey-adaptive-thresh-rising-en)
* [/sys/.../in\_capacitanceY\_adaptive\_thresh\_rising\_timeout](abi-testing.html#abi-sys-in-capacitancey-adaptive-thresh-rising-timeout)
* [/sys/.../in\_capacitanceY\_adaptive\_thresh\_falling\_timeout](abi-testing.html#abi-sys-in-capacitancey-adaptive-thresh-rising-timeout)
* [/sys/.../events/in\_accel\_mag\_adaptive\_rising\_value](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-value)
* [/sys/.../events/in\_accel\_thresh\_rising\_value](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-value)
* [/sys/.../events/in\_accel\_thresh\_falling\_value](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-value)
* [/sys/.../events/in\_accel\_x\_raw\_thresh\_rising\_value](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-value)
* [/sys/.../events/in\_accel\_x\_raw\_thresh\_falling\_value](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-value)
* [/sys/.../events/in\_accel\_y\_raw\_thresh\_rising\_value](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-value)
* [/sys/.../events/in\_accel\_y\_raw\_thresh\_falling\_value](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-value)
* [/sys/.../events/in\_accel\_z\_raw\_thresh\_rising\_value](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-value)
* [/sys/.../events/in\_accel\_z\_raw\_thresh\_falling\_value](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-value)
* [/sys/.../events/in\_anglvel\_x\_raw\_thresh\_rising\_value](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-value)
* [/sys/.../events/in\_anglvel\_x\_raw\_thresh\_falling\_value](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-value)
* [/sys/.../events/in\_anglvel\_y\_raw\_thresh\_rising\_value](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-value)
* [/sys/.../events/in\_anglvel\_y\_raw\_thresh\_falling\_value](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-value)
* [/sys/.../events/in\_anglvel\_z\_raw\_thresh\_rising\_value](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-value)
* [/sys/.../events/in\_anglvel\_z\_raw\_thresh\_falling\_value](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-value)
* [/sys/.../events/in\_magn\_x\_raw\_thresh\_rising\_value](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-value)
* [/sys/.../events/in\_magn\_x\_raw\_thresh\_falling\_value](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-value)
* [/sys/.../events/in\_magn\_y\_raw\_thresh\_rising\_value](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-value)
* [/sys/.../events/in\_magn\_y\_raw\_thresh\_falling\_value](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-value)
* [/sys/.../events/in\_magn\_z\_raw\_thresh\_rising\_value](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-value)
* [/sys/.../events/in\_magn\_z\_raw\_thresh\_falling\_value](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-value)
* [/sys/.../events/in\_rot\_from\_north\_magnetic\_raw\_thresh\_rising\_value](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-value)
* [/sys/.../events/in\_rot\_from\_north\_magnetic\_raw\_thresh\_falling\_value](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-value)
* [/sys/.../events/in\_rot\_from\_north\_true\_raw\_thresh\_rising\_value](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-value)
* [/sys/.../events/in\_rot\_from\_north\_true\_raw\_thresh\_falling\_value](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-value)
* [/sys/.../events/in\_rot\_from\_north\_magnetic\_tilt\_comp\_raw\_thresh\_rising\_value](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-value)
* [/sys/.../events/in\_rot\_from\_north\_magnetic\_tilt\_comp\_raw\_thresh\_falling\_value](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-value)
* [/sys/.../events/in\_rot\_from\_north\_true\_tilt\_comp\_raw\_thresh\_rising\_value](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-value)
* [/sys/.../events/in\_rot\_from\_north\_true\_tilt\_comp\_raw\_thresh\_falling\_value](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-value)
* [/sys/.../events/in\_voltageY\_supply\_raw\_thresh\_rising\_value](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-value)
* [/sys/.../events/in\_voltageY\_supply\_raw\_thresh\_falling\_value](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-value)
* [/sys/.../events/in\_voltageY\_raw\_thresh\_rising\_value](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-value)
* [/sys/.../events/in\_voltageY\_raw\_thresh\_falling\_value](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-value)
* [/sys/.../events/in\_tempY\_raw\_thresh\_rising\_value](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-value)
* [/sys/.../events/in\_tempY\_raw\_thresh\_falling\_value](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-value)
* [/sys/.../events/in\_illuminance0\_thresh\_falling\_value](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-value)
* [/sys/.../events/in\_illuminance0\_thresh\_rising\_value](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-value)
* [/sys/.../events/in\_proximity0\_thresh\_falling\_value](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-value)
* [/sys/.../events/in\_proximity0\_thresh\_rising\_value](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-value)
* [/sys/.../events/in\_illuminance\_thresh\_rising\_value](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-value)
* [/sys/.../events/in\_illuminance\_thresh\_falling\_value](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-value)
* [/sys/.../events/in\_capacitanceY\_thresh\_rising\_value](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-value)
* [/sys/.../events/in\_capacitanceY\_thresh\_falling\_value](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-value)
* [/sys/.../events/in\_capacitanceY\_thresh\_adaptive\_rising\_value](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-value)
* [/sys/.../events/in\_capacitanceY\_thresh\_falling\_rising\_value](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-value)
* [/sys/.../events/in\_pressure\_thresh\_rising\_value](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-value)
* [/sys/.../events/in\_accel\_scale](abi-testing.html#abi-sys-events-in-accel-scale)
* [/sys/.../events/in\_accel\_peak\_scale](abi-testing.html#abi-sys-events-in-accel-scale)
* [/sys/.../events/in\_anglvel\_scale](abi-testing.html#abi-sys-events-in-accel-scale)
* [/sys/.../events/in\_magn\_scale](abi-testing.html#abi-sys-events-in-accel-scale)
* [/sys/.../events/in\_rot\_from\_north\_magnetic\_scale](abi-testing.html#abi-sys-events-in-accel-scale)
* [/sys/.../events/in\_rot\_from\_north\_true\_scale](abi-testing.html#abi-sys-events-in-accel-scale)
* [/sys/.../events/in\_voltage\_scale](abi-testing.html#abi-sys-events-in-accel-scale)
* [/sys/.../events/in\_voltage\_supply\_scale](abi-testing.html#abi-sys-events-in-accel-scale)
* [/sys/.../events/in\_temp\_scale](abi-testing.html#abi-sys-events-in-accel-scale)
* [/sys/.../events/in\_illuminance\_scale](abi-testing.html#abi-sys-events-in-accel-scale)
* [/sys/.../events/in\_proximity\_scale](abi-testing.html#abi-sys-events-in-accel-scale)
* [/sys/.../events/in\_accel\_x\_thresh\_rising\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_accel\_x\_thresh\_falling\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_accel\_x\_thresh\_either\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_accel\_y\_thresh\_rising\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_accel\_y\_thresh\_falling\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_accel\_y\_thresh\_either\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_accel\_z\_thresh\_rising\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_accel\_z\_thresh\_falling\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_accel\_z\_thresh\_either\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_anglvel\_x\_thresh\_rising\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_anglvel\_x\_thresh\_falling\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_anglvel\_x\_thresh\_either\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_anglvel\_y\_thresh\_rising\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_anglvel\_y\_thresh\_falling\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_anglvel\_y\_thresh\_either\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_anglvel\_z\_thresh\_rising\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_anglvel\_z\_thresh\_falling\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_anglvel\_z\_thresh\_either\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_magn\_x\_thresh\_rising\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_magn\_x\_thresh\_falling\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_magn\_x\_thresh\_either\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_magn\_y\_thresh\_rising\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_magn\_y\_thresh\_falling\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_magn\_y\_thresh\_either\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_magn\_z\_thresh\_rising\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_magn\_z\_thresh\_falling\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_magn\_z\_thresh\_either\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_rot\_from\_north\_magnetic\_thresh\_rising\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_rot\_from\_north\_magnetic\_thresh\_falling\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_rot\_from\_north\_magnetic\_thresh\_either\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_rot\_from\_north\_true\_thresh\_rising\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_rot\_from\_north\_true\_thresh\_falling\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_rot\_from\_north\_true\_thresh\_either\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_rot\_from\_north\_magnetic\_tilt\_comp\_thresh\_rising\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_rot\_from\_north\_magnetic\_tilt\_comp\_thresh\_falling\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_rot\_from\_north\_magnetic\_tilt\_comp\_thresh\_either\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_rot\_from\_north\_true\_tilt\_comp\_thresh\_rising\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_rot\_from\_north\_true\_tilt\_comp\_thresh\_falling\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_rot\_from\_north\_true\_tilt\_comp\_thresh\_either\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_voltageY\_thresh\_rising\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_voltageY\_thresh\_falling\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_voltageY\_thresh\_either\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_tempY\_thresh\_rising\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_tempY\_thresh\_falling\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_tempY\_thresh\_either\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_illuminance0\_thresh\_falling\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_illuminance0\_thresh\_rising\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_illuminance0\_thresh\_either\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_proximity0\_thresh\_falling\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_proximity0\_thresh\_rising\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_proximity0\_thresh\_either\_hysteresis](abi-testing.html#abi-sys-events-in-accel-x-thresh-rising-hysteresis)
* [/sys/.../events/in\_accel\_roc\_rising\_value](abi-testing.html#abi-sys-events-in-accel-roc-rising-value)
* [/sys/.../events/in\_accel\_x\_raw\_roc\_rising\_value](abi-testing.html#abi-sys-events-in-accel-roc-rising-value)
* [/sys/.../events/in\_accel\_x\_raw\_roc\_falling\_value](abi-testing.html#abi-sys-events-in-accel-roc-rising-value)
* [/sys/.../events/in\_accel\_y\_raw\_roc\_rising\_value](abi-testing.html#abi-sys-events-in-accel-roc-rising-value)
* [/sys/.../events/in\_accel\_y\_raw\_roc\_falling\_value](abi-testing.html#abi-sys-events-in-accel-roc-rising-value)
* [/sys/.../events/in\_accel\_z\_raw\_roc\_rising\_value](abi-testing.html#abi-sys-events-in-accel-roc-rising-value)
* [/sys/.../events/in\_accel\_z\_raw\_roc\_falling\_value](abi-testing.html#abi-sys-events-in-accel-roc-rising-value)
* [/sys/.../events/in\_anglvel\_x\_raw\_roc\_rising\_value](abi-testing.html#abi-sys-events-in-accel-roc-rising-value)
* [/sys/.../events/in\_anglvel\_x\_raw\_roc\_falling\_value](abi-testing.html#abi-sys-events-in-accel-roc-rising-value)
* [/sys/.../events/in\_anglvel\_y\_raw\_roc\_rising\_value](abi-testing.html#abi-sys-events-in-accel-roc-rising-value)
* [/sys/.../events/in\_anglvel\_y\_raw\_roc\_falling\_value](abi-testing.html#abi-sys-events-in-accel-roc-rising-value)
* [/sys/.../events/in\_anglvel\_z\_raw\_roc\_rising\_value](abi-testing.html#abi-sys-events-in-accel-roc-rising-value)
* [/sys/.../events/in\_anglvel\_z\_raw\_roc\_falling\_value](abi-testing.html#abi-sys-events-in-accel-roc-rising-value)
* [/sys/.../events/in\_magn\_x\_raw\_roc\_rising\_value](abi-testing.html#abi-sys-events-in-accel-roc-rising-value)
* [/sys/.../events/in\_magn\_x\_raw\_roc\_falling\_value](abi-testing.html#abi-sys-events-in-accel-roc-rising-value)
* [/sys/.../events/in\_magn\_y\_raw\_roc\_rising\_value](abi-testing.html#abi-sys-events-in-accel-roc-rising-value)
* [/sys/.../events/in\_magn\_y\_raw\_roc\_falling\_value](abi-testing.html#abi-sys-events-in-accel-roc-rising-value)
* [/sys/.../events/in\_magn\_z\_raw\_roc\_rising\_value](abi-testing.html#abi-sys-events-in-accel-roc-rising-value)
* [/sys/.../events/in\_magn\_z\_raw\_roc\_falling\_value](abi-testing.html#abi-sys-events-in-accel-roc-rising-value)
* [/sys/.../events/in\_rot\_from\_north\_magnetic\_raw\_roc\_rising\_value](abi-testing.html#abi-sys-events-in-accel-roc-rising-value)
* [/sys/.../events/in\_rot\_from\_north\_magnetic\_raw\_roc\_falling\_value](abi-testing.html#abi-sys-events-in-accel-roc-rising-value)
* [/sys/.../events/in\_rot\_from\_north\_true\_raw\_roc\_rising\_value](abi-testing.html#abi-sys-events-in-accel-roc-rising-value)
* [/sys/.../events/in\_rot\_from\_north\_true\_raw\_roc\_falling\_value](abi-testing.html#abi-sys-events-in-accel-roc-rising-value)
* [/sys/.../events/in\_rot\_from\_north\_magnetic\_tilt\_comp\_raw\_roc\_rising\_value](abi-testing.html#abi-sys-events-in-accel-roc-rising-value)
* [/sys/.../events/in\_rot\_from\_north\_magnetic\_tilt\_comp\_raw\_roc\_falling\_value](abi-testing.html#abi-sys-events-in-accel-roc-rising-value)
* [/sys/.../events/in\_rot\_from\_north\_true\_tilt\_comp\_raw\_roc\_rising\_value](abi-testing.html#abi-sys-events-in-accel-roc-rising-value)
* [/sys/.../events/in\_rot\_from\_north\_true\_tilt\_comp\_raw\_roc\_falling\_value](abi-testing.html#abi-sys-events-in-accel-roc-rising-value)
* [/sys/.../events/in\_voltageY\_supply\_raw\_roc\_rising\_value](abi-testing.html#abi-sys-events-in-accel-roc-rising-value)
* [/sys/.../events/in\_voltageY\_supply\_raw\_roc\_falling\_value](abi-testing.html#abi-sys-events-in-accel-roc-rising-value)
* [/sys/.../events/in\_voltageY\_raw\_roc\_rising\_value](abi-testing.html#abi-sys-events-in-accel-roc-rising-value)
* [/sys/.../events/in\_voltageY\_raw\_roc\_falling\_value](abi-testing.html#abi-sys-events-in-accel-roc-rising-value)
* [/sys/.../events/in\_tempY\_raw\_roc\_rising\_value](abi-testing.html#abi-sys-events-in-accel-roc-rising-value)
* [/sys/.../events/in\_tempY\_raw\_roc\_falling\_value](abi-testing.html#abi-sys-events-in-accel-roc-rising-value)
* [/sys/.../events/in\_accel\_mag\_adaptive\_rising\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_accel\_roc\_rising\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_accel\_x\_thresh\_rising\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_accel\_x\_thresh\_falling\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_accel\_x\_roc\_rising\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_accel\_x\_roc\_falling\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_accel\_y\_thresh\_rising\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_accel\_y\_thresh\_falling\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_accel\_y\_roc\_rising\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_accel\_y\_roc\_falling\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_accel\_z\_thresh\_rising\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_accel\_z\_thresh\_falling\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_accel\_z\_roc\_rising\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_accel\_z\_roc\_falling\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_anglvel\_x\_thresh\_rising\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_anglvel\_x\_thresh\_falling\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_anglvel\_x\_roc\_rising\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_anglvel\_x\_roc\_falling\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_anglvel\_y\_thresh\_rising\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_anglvel\_y\_thresh\_falling\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_anglvel\_y\_roc\_rising\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_anglvel\_y\_roc\_falling\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_anglvel\_z\_thresh\_rising\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_anglvel\_z\_thresh\_falling\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_anglvel\_z\_roc\_rising\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_anglvel\_z\_roc\_falling\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_magn\_x\_thresh\_rising\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_magn\_x\_thresh\_falling\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_magn\_x\_roc\_rising\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_magn\_x\_roc\_falling\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_magn\_y\_thresh\_rising\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_magn\_y\_thresh\_falling\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_magn\_y\_roc\_rising\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_magn\_y\_roc\_falling\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_magn\_z\_thresh\_rising\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_magn\_z\_thresh\_falling\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_magn\_z\_roc\_rising\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_magn\_z\_roc\_falling\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_rot\_from\_north\_magnetic\_thresh\_rising\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_rot\_from\_north\_magnetic\_thresh\_falling\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_rot\_from\_north\_magnetic\_roc\_rising\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_rot\_from\_north\_magnetic\_roc\_falling\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_rot\_from\_north\_true\_thresh\_rising\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_rot\_from\_north\_true\_thresh\_falling\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_rot\_from\_north\_true\_roc\_rising\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_rot\_from\_north\_true\_roc\_falling\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_rot\_from\_north\_magnetic\_tilt\_comp\_thresh\_rising\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_rot\_from\_north\_magnetic\_tilt\_comp\_thresh\_falling\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_rot\_from\_north\_magnetic\_tilt\_comp\_roc\_rising\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_rot\_from\_north\_magnetic\_tilt\_comp\_roc\_falling\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_rot\_from\_north\_true\_tilt\_comp\_thresh\_rising\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_rot\_from\_north\_true\_tilt\_comp\_thresh\_falling\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_rot\_from\_north\_true\_tilt\_comp\_roc\_rising\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_rot\_from\_north\_true\_tilt\_comp\_roc\_falling\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_voltageY\_supply\_thresh\_rising\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_voltageY\_supply\_thresh\_falling\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_voltageY\_supply\_roc\_rising\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_voltageY\_supply\_roc\_falling\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_voltageY\_thresh\_rising\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_voltageY\_thresh\_falling\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_voltageY\_roc\_rising\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_voltageY\_roc\_falling\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_tempY\_thresh\_rising\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_tempY\_thresh\_falling\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_tempY\_roc\_rising\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_tempY\_roc\_falling\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_accel\_x&y&z\_mag\_falling\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_intensity0\_thresh\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_proximity0\_thresh\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_activity\_still\_thresh\_rising\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_activity\_still\_thresh\_falling\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_activity\_walking\_thresh\_rising\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_activity\_walking\_thresh\_falling\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_activity\_jogging\_thresh\_rising\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_activity\_jogging\_thresh\_falling\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_activity\_running\_thresh\_rising\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_activity\_running\_thresh\_falling\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_illuminance\_thresh\_either\_period](abi-testing.html#abi-sys-events-in-accel-mag-adaptive-rising-period)
* [/sys/.../events/in\_accel\_thresh\_rising\_low\_pass\_filter\_3db](abi-testing.html#abi-sys-events-in-accel-thresh-rising-low-pass-filter-3db)
* [/sys/.../events/in\_anglvel\_thresh\_rising\_low\_pass\_filter\_3db](abi-testing.html#abi-sys-events-in-accel-thresh-rising-low-pass-filter-3db)
* [/sys/.../events/in\_magn\_thresh\_rising\_low\_pass\_filter\_3db](abi-testing.html#abi-sys-events-in-accel-thresh-rising-low-pass-filter-3db)
* [/sys/.../events/in\_accel\_thresh\_rising\_high\_pass\_filter\_3db](abi-testing.html#abi-sys-events-in-accel-thresh-rising-high-pass-filter-3db)
* [/sys/.../events/in\_anglvel\_thresh\_rising\_high\_pass\_filter\_3db](abi-testing.html#abi-sys-events-in-accel-thresh-rising-high-pass-filter-3db)
* [/sys/.../events/in\_magn\_thresh\_rising\_high\_pass\_filter\_3db](abi-testing.html#abi-sys-events-in-accel-thresh-rising-high-pass-filter-3db)
* [/sys/.../events/in\_activity\_still\_thresh\_rising\_en](abi-testing.html#abi-sys-events-in-activity-still-thresh-rising-en)
* [/sys/.../events/in\_activity\_still\_thresh\_falling\_en](abi-testing.html#abi-sys-events-in-activity-still-thresh-rising-en)
* [/sys/.../events/in\_activity\_walking\_thresh\_rising\_en](abi-testing.html#abi-sys-events-in-activity-still-thresh-rising-en)
* [/sys/.../events/in\_activity\_walking\_thresh\_falling\_en](abi-testing.html#abi-sys-events-in-activity-still-thresh-rising-en)
* [/sys/.../events/in\_activity\_jogging\_thresh\_rising\_en](abi-testing.html#abi-sys-events-in-activity-still-thresh-rising-en)
* [/sys/.../events/in\_activity\_jogging\_thresh\_falling\_en](abi-testing.html#abi-sys-events-in-activity-still-thresh-rising-en)
* [/sys/.../events/in\_activity\_running\_thresh\_rising\_en](abi-testing.html#abi-sys-events-in-activity-still-thresh-rising-en)
* [/sys/.../events/in\_activity\_running\_thresh\_falling\_en](abi-testing.html#abi-sys-events-in-activity-still-thresh-rising-en)
* [/sys/.../events/in\_activity\_still\_thresh\_rising\_value](abi-testing.html#abi-sys-events-in-activity-still-thresh-rising-value)
* [/sys/.../events/in\_activity\_still\_thresh\_falling\_value](abi-testing.html#abi-sys-events-in-activity-still-thresh-rising-value)
* [/sys/.../events/in\_activity\_walking\_thresh\_rising\_value](abi-testing.html#abi-sys-events-in-activity-still-thresh-rising-value)
* [/sys/.../events/in\_activity\_walking\_thresh\_falling\_value](abi-testing.html#abi-sys-events-in-activity-still-thresh-rising-value)
* [/sys/.../events/in\_activity\_jogging\_thresh\_rising\_value](abi-testing.html#abi-sys-events-in-activity-still-thresh-rising-value)
* [/sys/.../events/in\_activity\_jogging\_thresh\_falling\_value](abi-testing.html#abi-sys-events-in-activity-still-thresh-rising-value)
* [/sys/.../events/in\_activity\_running\_thresh\_rising\_value](abi-testing.html#abi-sys-events-in-activity-still-thresh-rising-value)
* [/sys/.../events/in\_activity\_running\_thresh\_falling\_value](abi-testing.html#abi-sys-events-in-activity-still-thresh-rising-value)
* [/sys/.../iio:deviceX/events/in\_accel\_mag\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-mag-en)
* [/sys/.../iio:deviceX/events/in\_accel\_mag\_rising\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-mag-en)
* [/sys/.../iio:deviceX/events/in\_accel\_mag\_falling\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-mag-en)
* [/sys/.../iio:deviceX/events/in\_accel\_x\_mag\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-mag-en)
* [/sys/.../iio:deviceX/events/in\_accel\_x\_mag\_rising\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-mag-en)
* [/sys/.../iio:deviceX/events/in\_accel\_x\_mag\_falling\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-mag-en)
* [/sys/.../iio:deviceX/events/in\_accel\_y\_mag\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-mag-en)
* [/sys/.../iio:deviceX/events/in\_accel\_y\_mag\_rising\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-mag-en)
* [/sys/.../iio:deviceX/events/in\_accel\_y\_mag\_falling\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-mag-en)
* [/sys/.../iio:deviceX/events/in\_accel\_z\_mag\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-mag-en)
* [/sys/.../iio:deviceX/events/in\_accel\_z\_mag\_rising\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-mag-en)
* [/sys/.../iio:deviceX/events/in\_accel\_z\_mag\_falling\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-mag-en)
* [/sys/.../iio:deviceX/events/in\_accel\_x&y&z\_mag\_rising\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-mag-en)
* [/sys/.../iio:deviceX/events/in\_accel\_x&y&z\_mag\_falling\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-mag-en)
* [/sys/.../events/in\_accel\_raw\_mag\_value](abi-testing.html#abi-sys-events-in-accel-raw-mag-value)
* [/sys/.../events/in\_accel\_x\_raw\_mag\_rising\_value](abi-testing.html#abi-sys-events-in-accel-raw-mag-value)
* [/sys/.../events/in\_accel\_y\_raw\_mag\_rising\_value](abi-testing.html#abi-sys-events-in-accel-raw-mag-value)
* [/sys/.../events/in\_accel\_z\_raw\_mag\_rising\_value](abi-testing.html#abi-sys-events-in-accel-raw-mag-value)
* [/sys/.../iio:deviceX/events/in\_accel\_x\_mag\_adaptive\_rising\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-mag-adaptive-rising-en)
* [/sys/.../iio:deviceX/events/in\_accel\_y\_mag\_adaptive\_rising\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-mag-adaptive-rising-en)
* [/sys/.../iio:deviceX/events/in\_accel\_z\_mag\_adaptive\_rising\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-x-mag-adaptive-rising-en)
* [/sys/.../iio:deviceX/events/in\_accel\_mag\_referenced\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-mag-referenced-en)
* [/sys/.../iio:deviceX/events/in\_accel\_mag\_referenced\_rising\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-mag-referenced-en)
* [/sys/.../iio:deviceX/events/in\_accel\_mag\_referenced\_falling\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-mag-referenced-en)
* [/sys/.../iio:deviceX/events/in\_accel\_y\_mag\_referenced\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-mag-referenced-en)
* [/sys/.../iio:deviceX/events/in\_accel\_y\_mag\_referenced\_rising\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-mag-referenced-en)
* [/sys/.../iio:deviceX/events/in\_accel\_y\_mag\_referenced\_falling\_en](abi-testing.html#abi-sys-iio-devicex-events-in-accel-mag-referenced-en)
* [/sys/.../iio:deviceX/events/in\_accel\_mag\_referenced\_value](abi-testing.html#abi-sys-iio-devicex-events-in-accel-mag-referenced-value)
* [/sys/.../iio:deviceX/events/in\_accel\_mag\_referenced\_rising\_value](abi-testing.html#abi-sys-iio-devicex-events-in-accel-mag-referenced-value)
* [/sys/.../iio:deviceX/events/in\_accel\_mag\_referenced\_falling\_value](abi-testing.html#abi-sys-iio-devicex-events-in-accel-mag-referenced-value)
* [/sys/.../iio:deviceX/events/in\_accel\_y\_mag\_referenced\_value](abi-testing.html#abi-sys-iio-devicex-events-in-accel-mag-referenced-value)
* [/sys/.../iio:deviceX/events/in\_accel\_y\_mag\_referenced\_rising\_value](abi-testing.html#abi-sys-iio-devicex-events-in-accel-mag-referenced-value)
* [/sys/.../iio:deviceX/events/in\_accel\_y\_mag\_referenced\_falling\_value](abi-testing.html#abi-sys-iio-devicex-events-in-accel-mag-referenced-value)
* [/sys/.../events/in\_steps\_change\_en](abi-testing.html#abi-sys-events-in-steps-change-en)
* [/sys/.../events/in\_steps\_change\_value](abi-testing.html#abi-sys-events-in-steps-change-value)
* [/sys/bus/iio/devices/iio:deviceX/trigger/current\_trigger](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-trigger-current-trigger)
* [/sys/bus/iio/devices/iio:deviceX/bufferY/length](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-buffery-length)
* [/sys/bus/iio/devices/iio:deviceX/bufferY/enable](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-buffery-enable)
* [/sys/bus/iio/devices/iio:deviceX/bufferY](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-buffery)
* [/sys/.../iio:deviceX/bufferY/in\_accel\_x\_en](abi-testing.html#abi-sys-iio-devicex-buffery-in-accel-x-en)
* [/sys/.../iio:deviceX/bufferY/in\_accel\_y\_en](abi-testing.html#abi-sys-iio-devicex-buffery-in-accel-x-en)
* [/sys/.../iio:deviceX/bufferY/in\_accel\_z\_en](abi-testing.html#abi-sys-iio-devicex-buffery-in-accel-x-en)
* [/sys/.../iio:deviceX/bufferY/in\_deltaangl\_x\_en](abi-testing.html#abi-sys-iio-devicex-buffery-in-accel-x-en)
* [/sys/.../iio:deviceX/bufferY/in\_deltaangl\_y\_en](abi-testing.html#abi-sys-iio-devicex-buffery-in-accel-x-en)
* [/sys/.../iio:deviceX/bufferY/in\_deltaangl\_z\_en](abi-testing.html#abi-sys-iio-devicex-buffery-in-accel-x-en)
* [/sys/.../iio:deviceX/bufferY/in\_deltavelocity\_x\_en](abi-testing.html#abi-sys-iio-devicex-buffery-in-accel-x-en)
* [/sys/.../iio:deviceX/bufferY/in\_deltavelocity\_y\_en](abi-testing.html#abi-sys-iio-devicex-buffery-in-accel-x-en)
* [/sys/.../iio:deviceX/bufferY/in\_deltavelocity\_z\_en](abi-testing.html#abi-sys-iio-devicex-buffery-in-accel-x-en)
* [/sys/.../iio:deviceX/bufferY/in\_anglvel\_x\_en](abi-testing.html#abi-sys-iio-devicex-buffery-in-accel-x-en)
* [/sys/.../iio:deviceX/bufferY/in\_anglvel\_y\_en](abi-testing.html#abi-sys-iio-devicex-buffery-in-accel-x-en)
* [/sys/.../iio:deviceX/bufferY/in\_anglvel\_z\_en](abi-testing.html#abi-sys-iio-devicex-buffery-in-accel-x-en)
* [/sys/.../iio:deviceX/bufferY/in\_magn\_x\_en](abi-testing.html#abi-sys-iio-devicex-buffery-in-accel-x-en)
* [/sys/.../iio:deviceX/bufferY/in\_magn\_y\_en](abi-testing.html#abi-sys-iio-devicex-buffery-in-accel-x-en)
* [/sys/.../iio:deviceX/bufferY/in\_magn\_z\_en](abi-testing.html#abi-sys-iio-devicex-buffery-in-accel-x-en)
* [/sys/.../iio:deviceX/bufferY/in\_rot\_from\_north\_magnetic\_en](abi-testing.html#abi-sys-iio-devicex-buffery-in-accel-x-en)
* [/sys/.../iio:deviceX/bufferY/in\_rot\_from\_north\_true\_en](abi-testing.html#abi-sys-iio-devicex-buffery-in-accel-x-en)
* [/sys/.../iio:deviceX/bufferY/in\_rot\_from\_north\_magnetic\_tilt\_comp\_en](abi-testing.html#abi-sys-iio-devicex-buffery-in-accel-x-en)
* [/sys/.../iio:deviceX/bufferY/in\_rot\_from\_north\_true\_tilt\_comp\_en](abi-testing.html#abi-sys-iio-devicex-buffery-in-accel-x-en)
* [/sys/.../iio:deviceX/bufferY/in\_timestamp\_en](abi-testing.html#abi-sys-iio-devicex-buffery-in-accel-x-en)
* [/sys/.../iio:deviceX/bufferY/in\_voltageY\_supply\_en](abi-testing.html#abi-sys-iio-devicex-buffery-in-accel-x-en)
* [/sys/.../iio:deviceX/bufferY/in\_voltageY\_en](abi-testing.html#abi-sys-iio-devicex-buffery-in-accel-x-en)
* [/sys/.../iio:deviceX/bufferY/in\_voltageY-voltageZ\_en](abi-testing.html#abi-sys-iio-devicex-buffery-in-accel-x-en)
* [/sys/.../iio:deviceX/bufferY/in\_incli\_x\_en](abi-testing.html#abi-sys-iio-devicex-buffery-in-accel-x-en)
* [/sys/.../iio:deviceX/bufferY/in\_incli\_y\_en](abi-testing.html#abi-sys-iio-devicex-buffery-in-accel-x-en)
* [/sys/.../iio:deviceX/bufferY/in\_pressureY\_en](abi-testing.html#abi-sys-iio-devicex-buffery-in-accel-x-en)
* [/sys/.../iio:deviceX/bufferY/in\_pressure\_en](abi-testing.html#abi-sys-iio-devicex-buffery-in-accel-x-en)
* [/sys/.../iio:deviceX/bufferY/in\_rot\_quaternion\_en](abi-testing.html#abi-sys-iio-devicex-buffery-in-accel-x-en)
* [/sys/.../iio:deviceX/bufferY/in\_proximity\_en](abi-testing.html#abi-sys-iio-devicex-buffery-in-accel-x-en)
* [/sys/.../iio:deviceX/bufferY/in\_accel\_type](abi-testing.html#abi-sys-iio-devicex-buffery-in-accel-type)
* [/sys/.../iio:deviceX/bufferY/in\_deltaangl\_type](abi-testing.html#abi-sys-iio-devicex-buffery-in-accel-type)
* [/sys/.../iio:deviceX/bufferY/in\_deltavelocity\_type](abi-testing.html#abi-sys-iio-devicex-buffery-in-accel-type)
* [/sys/.../iio:deviceX/bufferY/in\_anglvel\_type](abi-testing.html#abi-sys-iio-devicex-buffery-in-accel-type)
* [/sys/.../iio:deviceX/bufferY/in\_magn\_type](abi-testing.html#abi-sys-iio-devicex-buffery-in-accel-type)
* [/sys/.../iio:deviceX/bufferY/in\_incli\_type](abi-testing.html#abi-sys-iio-devicex-buffery-in-accel-type)
* [/sys/.../iio:deviceX/bufferY/in\_voltageY\_type](abi-testing.html#abi-sys-iio-devicex-buffery-in-accel-type)
* [/sys/.../iio:deviceX/bufferY/in\_voltage\_type](abi-testing.html#abi-sys-iio-devicex-buffery-in-accel-type)
* [/sys/.../iio:deviceX/bufferY/in\_voltageY\_supply\_type](abi-testing.html#abi-sys-iio-devicex-buffery-in-accel-type)
* [/sys/.../iio:deviceX/bufferY/in\_timestamp\_type](abi-testing.html#abi-sys-iio-devicex-buffery-in-accel-type)
* [/sys/.../iio:deviceX/bufferY/in\_pressureY\_type](abi-testing.html#abi-sys-iio-devicex-buffery-in-accel-type)
* [/sys/.../iio:deviceX/bufferY/in\_pressure\_type](abi-testing.html#abi-sys-iio-devicex-buffery-in-accel-type)
* [/sys/.../iio:deviceX/bufferY/in\_rot\_quaternion\_type](abi-testing.html#abi-sys-iio-devicex-buffery-in-accel-type)
* [/sys/.../iio:deviceX/bufferY/in\_proximity\_type](abi-testing.html#abi-sys-iio-devicex-buffery-in-accel-type)
* [/sys/.../iio:deviceX/scan\_elements/in\_accel\_type\_available](abi-testing.html#abi-sys-iio-devicex-scan-elements-in-accel-type-available)
* [/sys/.../iio:deviceX/bufferY/in\_voltageY\_index](abi-testing.html#abi-sys-iio-devicex-buffery-in-voltagey-index)
* [/sys/.../iio:deviceX/bufferY/in\_voltageY\_supply\_index](abi-testing.html#abi-sys-iio-devicex-buffery-in-voltagey-index)
* [/sys/.../iio:deviceX/bufferY/in\_accel\_x\_index](abi-testing.html#abi-sys-iio-devicex-buffery-in-voltagey-index)
* [/sys/.../iio:deviceX/bufferY/in\_accel\_y\_index](abi-testing.html#abi-sys-iio-devicex-buffery-in-voltagey-index)
* [/sys/.../iio:deviceX/bufferY/in\_accel\_z\_index](abi-testing.html#abi-sys-iio-devicex-buffery-in-voltagey-index)
* [/sys/.../iio:deviceX/bufferY/in\_deltaangl\_x\_index](abi-testing.html#abi-sys-iio-devicex-buffery-in-voltagey-index)
* [/sys/.../iio:deviceX/bufferY/in\_deltaangl\_y\_index](abi-testing.html#abi-sys-iio-devicex-buffery-in-voltagey-index)
* [/sys/.../iio:deviceX/bufferY/in\_deltaangl\_z\_index](abi-testing.html#abi-sys-iio-devicex-buffery-in-voltagey-index)
* [/sys/.../iio:deviceX/bufferY/in\_deltavelocity\_x\_index](abi-testing.html#abi-sys-iio-devicex-buffery-in-voltagey-index)
* [/sys/.../iio:deviceX/bufferY/in\_deltavelocity\_y\_index](abi-testing.html#abi-sys-iio-devicex-buffery-in-voltagey-index)
* [/sys/.../iio:deviceX/bufferY/in\_deltavelocity\_z\_index](abi-testing.html#abi-sys-iio-devicex-buffery-in-voltagey-index)
* [/sys/.../iio:deviceX/bufferY/in\_anglvel\_x\_index](abi-testing.html#abi-sys-iio-devicex-buffery-in-voltagey-index)
* [/sys/.../iio:deviceX/bufferY/in\_anglvel\_y\_index](abi-testing.html#abi-sys-iio-devicex-buffery-in-voltagey-index)
* [/sys/.../iio:deviceX/bufferY/in\_anglvel\_z\_index](abi-testing.html#abi-sys-iio-devicex-buffery-in-voltagey-index)
* [/sys/.../iio:deviceX/bufferY/in\_magn\_x\_index](abi-testing.html#abi-sys-iio-devicex-buffery-in-voltagey-index)
* [/sys/.../iio:deviceX/bufferY/in\_magn\_y\_index](abi-testing.html#abi-sys-iio-devicex-buffery-in-voltagey-index)
* [/sys/.../iio:deviceX/bufferY/in\_magn\_z\_index](abi-testing.html#abi-sys-iio-devicex-buffery-in-voltagey-index)
* [/sys/.../iio:deviceX/bufferY/in\_rot\_from\_north\_magnetic\_index](abi-testing.html#abi-sys-iio-devicex-buffery-in-voltagey-index)
* [/sys/.../iio:deviceX/bufferY/in\_rot\_from\_north\_true\_index](abi-testing.html#abi-sys-iio-devicex-buffery-in-voltagey-index)
* [/sys/.../iio:deviceX/bufferY/in\_rot\_from\_north\_magnetic\_tilt\_comp\_index](abi-testing.html#abi-sys-iio-devicex-buffery-in-voltagey-index)
* [/sys/.../iio:deviceX/bufferY/in\_rot\_from\_north\_true\_tilt\_comp\_index](abi-testing.html#abi-sys-iio-devicex-buffery-in-voltagey-index)
* [/sys/.../iio:deviceX/bufferY/in\_incli\_x\_index](abi-testing.html#abi-sys-iio-devicex-buffery-in-voltagey-index)
* [/sys/.../iio:deviceX/bufferY/in\_incli\_y\_index](abi-testing.html#abi-sys-iio-devicex-buffery-in-voltagey-index)
* [/sys/.../iio:deviceX/bufferY/in\_timestamp\_index](abi-testing.html#abi-sys-iio-devicex-buffery-in-voltagey-index)
* [/sys/.../iio:deviceX/bufferY/in\_pressureY\_index](abi-testing.html#abi-sys-iio-devicex-buffery-in-voltagey-index)
* [/sys/.../iio:deviceX/bufferY/in\_pressure\_index](abi-testing.html#abi-sys-iio-devicex-buffery-in-voltagey-index)
* [/sys/.../iio:deviceX/bufferY/in\_rot\_quaternion\_index](abi-testing.html#abi-sys-iio-devicex-buffery-in-voltagey-index)
* [/sys/.../iio:deviceX/bufferY/in\_proximity\_index](abi-testing.html#abi-sys-iio-devicex-buffery-in-voltagey-index)
* [/sys/.../iio:deviceX/in\_activity\_still\_input](abi-testing.html#abi-sys-iio-devicex-in-activity-still-input)
* [/sys/.../iio:deviceX/in\_activity\_walking\_input](abi-testing.html#abi-sys-iio-devicex-in-activity-still-input)
* [/sys/.../iio:deviceX/in\_activity\_jogging\_input](abi-testing.html#abi-sys-iio-devicex-in-activity-still-input)
* [/sys/.../iio:deviceX/in\_activity\_running\_input](abi-testing.html#abi-sys-iio-devicex-in-activity-still-input)
* [/sys/.../iio:deviceX/in\_anglvel\_z\_quadrature\_correction\_raw](abi-testing.html#abi-sys-iio-devicex-in-anglvel-z-quadrature-correction-raw)
* [/sys/.../iio:deviceX/in\_accelY\_power\_mode](abi-testing.html#abi-sys-iio-devicex-in-accely-power-mode)
* [/sys/.../iio:deviceX/in\_energy\_input](abi-testing.html#abi-sys-iio-devicex-in-energy-input)
* [/sys/.../iio:deviceX/in\_energy\_raw](abi-testing.html#abi-sys-iio-devicex-in-energy-input)
* [/sys/.../iio:deviceX/in\_energyY\_active\_raw](abi-testing.html#abi-sys-iio-devicex-in-energy-input)
* [/sys/.../iio:deviceX/in\_energyY\_reactive\_raw](abi-testing.html#abi-sys-iio-devicex-in-energy-input)
* [/sys/.../iio:deviceX/in\_energyY\_apparent\_raw](abi-testing.html#abi-sys-iio-devicex-in-energy-input)
* [/sys/.../iio:deviceX/in\_distance\_input](abi-testing.html#abi-sys-iio-devicex-in-distance-input)
* [/sys/.../iio:deviceX/in\_distance\_raw](abi-testing.html#abi-sys-iio-devicex-in-distance-input)
* [/sys/bus/iio/devices/iio:deviceX/store\_eeprom](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-store-eeprom)
* [/sys/.../iio:deviceX/in\_proximity\_raw](abi-testing.html#abi-sys-iio-devicex-in-proximity-raw)
* [/sys/.../iio:deviceX/in\_proximity\_input](abi-testing.html#abi-sys-iio-devicex-in-proximity-raw)
* [/sys/.../iio:deviceX/in\_proximityY\_raw](abi-testing.html#abi-sys-iio-devicex-in-proximity-raw)
* [/sys/.../iio:deviceX/in\_illuminance\_input](abi-testing.html#abi-sys-iio-devicex-in-illuminance-input)
* [/sys/.../iio:deviceX/in\_illuminance\_raw](abi-testing.html#abi-sys-iio-devicex-in-illuminance-input)
* [/sys/.../iio:deviceX/in\_illuminanceY\_input](abi-testing.html#abi-sys-iio-devicex-in-illuminance-input)
* [/sys/.../iio:deviceX/in\_illuminanceY\_raw](abi-testing.html#abi-sys-iio-devicex-in-illuminance-input)
* [/sys/.../iio:deviceX/in\_illuminanceY\_mean\_raw](abi-testing.html#abi-sys-iio-devicex-in-illuminance-input)
* [/sys/.../iio:deviceX/in\_illuminance\_ir\_raw](abi-testing.html#abi-sys-iio-devicex-in-illuminance-input)
* [/sys/.../iio:deviceX/in\_illuminance\_clear\_raw](abi-testing.html#abi-sys-iio-devicex-in-illuminance-input)
* [/sys/.../iio:deviceX/in\_intensityY\_raw](abi-testing.html#abi-sys-iio-devicex-in-intensityy-raw)
* [/sys/.../iio:deviceX/in\_intensityY\_ir\_raw](abi-testing.html#abi-sys-iio-devicex-in-intensityy-raw)
* [/sys/.../iio:deviceX/in\_intensityY\_both\_raw](abi-testing.html#abi-sys-iio-devicex-in-intensityy-raw)
* [/sys/.../iio:deviceX/in\_intensityY\_uv\_raw](abi-testing.html#abi-sys-iio-devicex-in-intensityy-raw)
* [/sys/.../iio:deviceX/in\_intensityY\_uva\_raw](abi-testing.html#abi-sys-iio-devicex-in-intensityy-raw)
* [/sys/.../iio:deviceX/in\_intensityY\_uvb\_raw](abi-testing.html#abi-sys-iio-devicex-in-intensityy-raw)
* [/sys/.../iio:deviceX/in\_intensityY\_duv\_raw](abi-testing.html#abi-sys-iio-devicex-in-intensityy-raw)
* [/sys/.../iio:deviceX/in\_intensity\_red\_raw](abi-testing.html#abi-sys-iio-devicex-in-intensityy-raw)
* [/sys/.../iio:deviceX/in\_intensity\_green\_raw](abi-testing.html#abi-sys-iio-devicex-in-intensityy-raw)
* [/sys/.../iio:deviceX/in\_intensity\_blue\_raw](abi-testing.html#abi-sys-iio-devicex-in-intensityy-raw)
* [/sys/.../iio:deviceX/in\_intensity\_clear\_raw](abi-testing.html#abi-sys-iio-devicex-in-intensityy-raw)
* [/sys/.../iio:deviceX/in\_uvindex\_input](abi-testing.html#abi-sys-iio-devicex-in-uvindex-input)
* [/sys/.../iio:deviceX/in\_intensity\_integration\_time](abi-testing.html#abi-sys-iio-devicex-in-intensity-integration-time)
* [/sys/.../iio:deviceX/in\_intensity\_red\_integration\_time](abi-testing.html#abi-sys-iio-devicex-in-intensity-integration-time)
* [/sys/.../iio:deviceX/in\_intensity\_green\_integration\_time](abi-testing.html#abi-sys-iio-devicex-in-intensity-integration-time)
* [/sys/.../iio:deviceX/in\_intensity\_blue\_integration\_time](abi-testing.html#abi-sys-iio-devicex-in-intensity-integration-time)
* [/sys/.../iio:deviceX/in\_intensity\_clear\_integration\_time](abi-testing.html#abi-sys-iio-devicex-in-intensity-integration-time)
* [/sys/.../iio:deviceX/in\_illuminance\_integration\_time](abi-testing.html#abi-sys-iio-devicex-in-intensity-integration-time)
* [/sys/.../iio:deviceX/in\_velocity\_sqrt(x^2+y^2+z^2)\_integration\_time](abi-testing.html#abi-sys-iio-devicex-in-velocity-sqrt-x-2-y-2-z-2-integration-time)
* [/sys/bus/iio/devices/iio:deviceX/in\_rot\_quaternion\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-rot-quaternion-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_rot\_from\_north\_magnetic\_tilt\_comp\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-rot-from-north-magnetic-tilt-comp-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_rot\_from\_north\_true\_tilt\_comp\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-rot-from-north-magnetic-tilt-comp-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_rot\_from\_north\_magnetic\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-rot-from-north-magnetic-tilt-comp-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_rot\_from\_north\_true\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-rot-from-north-magnetic-tilt-comp-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_currentY\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-currenty-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_currentY\_supply\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-currenty-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_altcurrentY\_rms\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-altcurrenty-rms-raw)
* [/sys/.../iio:deviceX/in\_energy\_en](abi-testing.html#abi-sys-iio-devicex-in-energy-en)
* [/sys/.../iio:deviceX/in\_distance\_en](abi-testing.html#abi-sys-iio-devicex-in-energy-en)
* [/sys/.../iio:deviceX/in\_velocity\_sqrt(x^2+y^2+z^2)\_en](abi-testing.html#abi-sys-iio-devicex-in-energy-en)
* [/sys/.../iio:deviceX/in\_steps\_en](abi-testing.html#abi-sys-iio-devicex-in-energy-en)
* [/sys/.../iio:deviceX/in\_steps\_input](abi-testing.html#abi-sys-iio-devicex-in-steps-input)
* [/sys/.../iio:deviceX/in\_velocity\_sqrt(x^2+y^2+z^2)\_input](abi-testing.html#abi-sys-iio-devicex-in-velocity-sqrt-x-2-y-2-z-2-input)
* [/sys/.../iio:deviceX/in\_velocity\_sqrt(x^2+y^2+z^2)\_raw](abi-testing.html#abi-sys-iio-devicex-in-velocity-sqrt-x-2-y-2-z-2-input)
* [/sys/.../iio:deviceX/in\_steps\_debounce\_count](abi-testing.html#abi-sys-iio-devicex-in-steps-debounce-count)
* [/sys/.../iio:deviceX/in\_steps\_debounce\_time](abi-testing.html#abi-sys-iio-devicex-in-steps-debounce-time)
* [/sys/bus/iio/devices/iio:deviceX/bufferY/watermark](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-buffery-watermark)
* [/sys/bus/iio/devices/iio:deviceX/bufferY/data\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-buffery-data-available)
* [/sys/bus/iio/devices/iio:deviceX/buffer/hwfifo\_enabled](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-buffer-hwfifo-enabled)
* [/sys/bus/iio/devices/iio:device\*/buffer/hwfifo\_timeout](abi-testing.html#abi-sys-bus-iio-devices-iio-device-buffer-hwfifo-timeout)
* [/sys/bus/iio/devices/iio:deviceX/buffer/hwfifo\_watermark](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-buffer-hwfifo-watermark)
* [/sys/bus/iio/devices/iio:deviceX/buffer/hwfifo\_watermark\_min](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-buffer-hwfifo-watermark-min)
* [/sys/bus/iio/devices/iio:deviceX/buffer/hwfifo\_watermark\_max](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-buffer-hwfifo-watermark-max)
* [/sys/bus/iio/devices/iio:deviceX/buffer/hwfifo\_watermark\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-buffer-hwfifo-watermark-available)
* [/sys/bus/iio/devices/iio:deviceX/in\_temp\_calibemissivity](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-temp-calibemissivity)
* [/sys/bus/iio/devices/iio:deviceX/in\_tempY\_calibemissivity](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-temp-calibemissivity)
* [/sys/bus/iio/devices/iio:deviceX/in\_temp\_object\_calibemissivity](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-temp-calibemissivity)
* [/sys/bus/iio/devices/iio:deviceX/in\_tempY\_object\_calibemissivity](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-temp-calibemissivity)
* [/sys/bus/iio/devices/iio:deviceX/in\_magn\_x\_oversampling\_ratio](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-magn-x-oversampling-ratio)
* [/sys/bus/iio/devices/iio:deviceX/in\_magn\_y\_oversampling\_ratio](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-magn-x-oversampling-ratio)
* [/sys/bus/iio/devices/iio:deviceX/in\_magn\_z\_oversampling\_ratio](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-magn-x-oversampling-ratio)
* [/sys/bus/iio/devices/iio:deviceX/in\_concentration\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-concentration-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_concentrationY\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-concentration-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_concentration\_co2\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-concentration-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_concentrationY\_co2\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-concentration-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_concentration\_ethanol\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-concentration-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_concentrationY\_ethanol\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-concentration-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_concentration\_h2\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-concentration-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_concentrationY\_h2\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-concentration-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_concentration\_o2\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-concentration-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_concentrationY\_o2\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-concentration-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_concentration\_voc\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-concentration-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_concentrationY\_voc\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-concentration-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_resistance\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-resistance-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_resistanceY\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-resistance-raw)
* [/sys/bus/iio/devices/iio:deviceX/out\_resistance\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-resistance-raw)
* [/sys/bus/iio/devices/iio:deviceX/out\_resistanceY\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-resistance-raw)
* [/sys/bus/iio/devices/iio:deviceX/heater\_enable](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-heater-enable)
* [/sys/bus/iio/devices/iio:deviceX/in\_ph\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-ph-raw)
* [/sys/bus/iio/devices/iio:deviceX/mount\_matrix](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-mount-matrix)
* [/sys/bus/iio/devices/iio:deviceX/in\_mount\_matrix](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-mount-matrix)
* [/sys/bus/iio/devices/iio:deviceX/out\_mount\_matrix](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-mount-matrix)
* [/sys/bus/iio/devices/iio:deviceX/in\_anglvel\_mount\_matrix](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-mount-matrix)
* [/sys/bus/iio/devices/iio:deviceX/in\_accel\_mount\_matrix](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-mount-matrix)
* [/sys/bus/iio/devices/iio:deviceX/in\_electricalconductivity\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-electricalconductivity-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_countY\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-county-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_indexY\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-indexy-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_count\_count\_direction\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-count-count-direction-available)
* [/sys/bus/iio/devices/iio:deviceX/in\_countY\_count\_direction](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-county-count-direction)
* [/sys/bus/iio/devices/iio:deviceX/in\_voltageY\_label](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-label)
* [/sys/bus/iio/devices/iio:deviceX/out\_voltageY\_label](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-label)
* [/sys/bus/iio/devices/iio:deviceX/in\_phaseY\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-phasey-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_massconcentration\_pm1\_input](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-massconcentration-pm1-input)
* [/sys/bus/iio/devices/iio:deviceX/in\_massconcentrationY\_pm1\_input](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-massconcentration-pm1-input)
* [/sys/bus/iio/devices/iio:deviceX/in\_massconcentration\_pm2p5\_input](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-massconcentration-pm1-input)
* [/sys/bus/iio/devices/iio:deviceX/in\_massconcentrationY\_pm2p5\_input](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-massconcentration-pm1-input)
* [/sys/bus/iio/devices/iio:deviceX/in\_massconcentration\_pm4\_input](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-massconcentration-pm1-input)
* [/sys/bus/iio/devices/iio:deviceX/in\_massconcentrationY\_pm4\_input](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-massconcentration-pm1-input)
* [/sys/bus/iio/devices/iio:deviceX/in\_massconcentration\_pm10\_input](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-massconcentration-pm1-input)
* [/sys/bus/iio/devices/iio:deviceX/in\_massconcentrationY\_pm10\_input](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-massconcentration-pm1-input)
* [/sys/bus/iio/devices/iio:deviceX/events/in\_illuminance\_period\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-events-in-illuminance-period-available)
* [/sys/bus/iio/devices/iio:deviceX/in\_filter\_notch\_center\_frequency](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-filter-notch-center-frequency)
* [/sys/bus/iio/devices/iio:deviceX/in\_temp\_thermocouple\_type](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-temp-thermocouple-type)
* [/sys/bus/iio/devices/iio:deviceX/in\_temp\_object\_calibambient](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-temp-object-calibambient)
* [/sys/bus/iio/devices/iio:deviceX/in\_tempY\_object\_calibambient](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-temp-object-calibambient)
* [/sys/bus/iio/devices/iio:deviceX/in\_intensity\_x\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-intensity-x-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_intensity\_y\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-intensity-x-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_intensity\_z\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-intensity-x-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_anglY\_label](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-angly-label)
* [/sys/bus/iio/devices/iio:deviceX/in\_illuminance\_hysteresis\_relative](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-illuminance-hysteresis-relative)
* [/sys/bus/iio/devices/iio:deviceX/in\_intensity\_hysteresis\_relative](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-illuminance-hysteresis-relative)
* [/sys/bus/iio/devices/iio:deviceX/calibration\_auto\_enable](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-calibration-auto-enable)
* [/sys/bus/iio/devices/iio:deviceX/calibration\_forced\_value](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-calibration-forced-value)
* [/sys/bus/iio/devices/iio:deviceX/calibration\_forced\_value\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-calibration-forced-value-available)
* [/sys/bus/iio/devices/iio:deviceX/in\_voltageY\_sampling\_frequency](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-sampling-frequency)
* [/sys/bus/iio/devices/iio:deviceX/in\_powerY\_sampling\_frequency](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-sampling-frequency)
* [/sys/bus/iio/devices/iio:deviceX/in\_currentY\_sampling\_frequency](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-sampling-frequency)
* [/sys/.../events/in\_accel\_gesture\_singletap\_en](abi-testing.html#abi-sys-events-in-accel-gesture-singletap-en)
* [/sys/.../events/in\_accel\_gesture\_doubletap\_en](abi-testing.html#abi-sys-events-in-accel-gesture-singletap-en)
* [/sys/.../events/in\_accel\_gesture\_singletap\_value](abi-testing.html#abi-sys-events-in-accel-gesture-singletap-value)
* [/sys/.../events/in\_accel\_gesture\_doubletap\_value](abi-testing.html#abi-sys-events-in-accel-gesture-singletap-value)
* [/sys/.../events/in\_accel\_gesture\_tap\_value\_available](abi-testing.html#abi-sys-events-in-accel-gesture-tap-value-available)
* [/sys/.../events/in\_accel\_gesture\_singletap\_reset\_timeout](abi-testing.html#abi-sys-events-in-accel-gesture-singletap-reset-timeout)
* [/sys/.../events/in\_accel\_gesture\_doubletap\_reset\_timeout](abi-testing.html#abi-sys-events-in-accel-gesture-singletap-reset-timeout)
* [/sys/.../events/in\_accel\_gesture\_tap\_reset\_timeout\_available](abi-testing.html#abi-sys-events-in-accel-gesture-tap-reset-timeout-available)
* [/sys/.../events/in\_accel\_gesture\_doubletap\_tap2\_min\_delay](abi-testing.html#abi-sys-events-in-accel-gesture-doubletap-tap2-min-delay)
* [/sys/.../events/in\_accel\_gesture\_doubletap\_tap2\_min\_delay\_available](abi-testing.html#abi-sys-events-in-accel-gesture-doubletap-tap2-min-delay-available)
* [/sys/.../events/in\_accel\_gesture\_tap\_maxtomin\_time](abi-testing.html#abi-sys-events-in-accel-gesture-tap-maxtomin-time)
* [/sys/.../events/in\_accel\_gesture\_tap\_maxtomin\_time\_available](abi-testing.html#abi-sys-events-in-accel-gesture-tap-maxtomin-time-available)
* [/sys/bus/iio/devices/iio:deviceX/in\_rot\_yaw\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-rot-yaw-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_rot\_pitch\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-rot-yaw-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_rot\_roll\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-rot-yaw-raw)
* [/sys/bus/iio/devices/iio:deviceX/serialnumber](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-serialnumber)
* [/sys/bus/iio/devices/iio:deviceX/filter\_type\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-filter-type-available)
* [/sys/bus/iio/devices/iio:deviceX/in\_voltage-voltage\_filter\_type\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-filter-type-available)
* [/sys/bus/iio/devices/iio:deviceX/filter\_type](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-filter-type)
* [/sys/bus/iio/devices/iio:deviceX/in\_voltageY-voltageZ\_filter\_type](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-filter-type)
* [/sys/.../events/in\_proximity\_thresh\_either\_runningperiod](abi-testing.html#abi-sys-events-in-proximity-thresh-either-runningperiod)
* [/sys/.../events/in\_proximity\_thresh\_either\_runningcount](abi-testing.html#abi-sys-events-in-proximity-thresh-either-runningcount)
* [/sys/bus/iio/devices/iio:deviceX/in\_colortemp\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-colortemp-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_chromaticity\_x\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-chromaticity-x-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_chromaticity\_y\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-chromaticity-x-raw)
* [/sys/bus/iio/devices/iio:deviceX/events/in\_altvoltageY\_mag\_either\_label](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-events-in-altvoltagey-mag-either-label)
* [/sys/bus/iio/devices/iio:deviceX/events/in\_altvoltageY\_mag\_rising\_label](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-events-in-altvoltagey-mag-either-label)
* [/sys/bus/iio/devices/iio:deviceX/events/in\_altvoltageY\_thresh\_falling\_label](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-events-in-altvoltagey-mag-either-label)
* [/sys/bus/iio/devices/iio:deviceX/events/in\_altvoltageY\_thresh\_rising\_label](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-events-in-altvoltagey-mag-either-label)
* [/sys/bus/iio/devices/iio:deviceX/events/in\_anglvelY\_mag\_rising\_label](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-events-in-altvoltagey-mag-either-label)
* [/sys/bus/iio/devices/iio:deviceX/events/in\_anglY\_thresh\_rising\_label](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-events-in-altvoltagey-mag-either-label)
* [/sys/bus/iio/devices/iio:deviceX/events/in\_phaseY\_mag\_rising\_label](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-events-in-altvoltagey-mag-either-label)
* [/sys/.../events/in\_accel\_gesture\_tap\_wait\_timeout](abi-testing.html#abi-sys-events-in-accel-gesture-tap-wait-timeout)
* [/sys/.../events/in\_accel\_gesture\_tap\_wait\_dur](abi-testing.html#abi-sys-events-in-accel-gesture-tap-wait-dur)
* [/sys/.../events/in\_accel\_gesture\_tap\_wait\_dur\_available](abi-testing.html#abi-sys-events-in-accel-gesture-tap-wait-dur-available)
* [/sys/.../iio:deviceX/in\_shunt\_resistor](abi-testing.html#abi-sys-iio-devicex-in-shunt-resistor)
* [/sys/.../iio:deviceX/in\_current\_shunt\_resistor](abi-testing.html#abi-sys-iio-devicex-in-shunt-resistor)
* [/sys/.../iio:deviceX/in\_power\_shunt\_resistor](abi-testing.html#abi-sys-iio-devicex-in-shunt-resistor)
* [/sys/.../iio:deviceX/in\_attention\_input](abi-testing.html#abi-sys-iio-devicex-in-attention-input)
* [/sys/.../events/in\_accel\_value\_available](abi-testing.html#abi-sys-events-in-accel-value-available)
* [/sys/.../events/in\_accel\_period\_available](abi-testing.html#abi-sys-events-in-accel-period-available)

## ABI file testing/sysfs-bus-iio-accel-adxl372

Has the following ABI:

* [/sys/bus/iio/devices/triggerX/name = “adxl372-devX-peak”](abi-testing.html#abi-sys-bus-iio-devices-triggerx-name-adxl372-devx-peak)

## ABI file testing/sysfs-bus-iio-accel-bmc150

Has the following ABI:

* [/sys/bus/iio/devices/triggerX/name = “bmc150\_accel-any-motion-devX”](abi-testing.html#abi-sys-bus-iio-devices-triggerx-name-bmc150-accel-any-motion-devx)

## ABI file testing/sysfs-bus-iio-ad9739a

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/out\_voltageY\_operating\_mode](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-voltagey-operating-mode)
* [/sys/bus/iio/devices/iio:deviceX/out\_voltageY\_operating\_mode\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-voltagey-operating-mode-available)

## ABI file testing/sysfs-bus-iio-adc-ad-sigma-delta

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/in\_voltageY\_sys\_calibration](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-sys-calibration)
* [/sys/bus/iio/devices/iio:deviceX/in\_voltageY\_sys\_calibration\_mode\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-sys-calibration-mode-available)
* [/sys/bus/iio/devices/iio:deviceX/in\_voltageY\_sys\_calibration\_mode](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-sys-calibration-mode)

## ABI file testing/sysfs-bus-iio-adc-ad4130

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/in\_voltage-voltage\_filter\_mode\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltage-voltage-filter-mode-available)
* [/sys/bus/iio/devices/iio:deviceX/in\_voltageY-voltageZ\_filter\_mode](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-voltagez-filter-mode)

## ABI file testing/sysfs-bus-iio-adc-ad7192

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/ac\_excitation\_en](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-ac-excitation-en)
* [/sys/bus/iio/devices/iio:deviceX/bridge\_switch\_en](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-bridge-switch-en)
* [/sys/bus/iio/devices/iio:deviceX/in\_voltage2-voltage2\_shorted\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltage2-voltage2-shorted-raw)

## ABI file testing/sysfs-bus-iio-adc-ad7280a

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/in\_voltageY-voltageZ\_balance\_switch\_en](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-voltagez-balance-switch-en)
* [/sys/bus/iio/devices/iio:deviceX/in\_voltageY-voltageZ\_balance\_switch\_timer](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-voltagez-balance-switch-timer)

## ABI file testing/sysfs-bus-iio-adc-envelope-detector

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/in\_altvoltageY\_invert](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-altvoltagey-invert)
* [/sys/bus/iio/devices/iio:deviceX/in\_altvoltageY\_compare\_interval](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-altvoltagey-compare-interval)

## ABI file testing/sysfs-bus-iio-adc-hi8435

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/in\_voltageY\_sensing\_mode](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-sensing-mode)
* [/sys/bus/iio/devices/iio:deviceX/events/in\_voltageY\_thresh\_falling\_value](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-events-in-voltagey-thresh-falling-value)
* [/sys/bus/iio/devices/iio:deviceX/events/in\_voltageY\_thresh\_rising\_value](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-events-in-voltagey-thresh-rising-value)

## ABI file testing/sysfs-bus-iio-adc-max11410

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/in\_voltage\_filterY\_notch\_en](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltage-filtery-notch-en)
* [/sys/bus/iio/devices/iio:deviceX/in\_voltage\_filterY\_notch\_center](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltage-filtery-notch-center)

## ABI file testing/sysfs-bus-iio-adc-mcp3564

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/boost\_current\_gain](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-boost-current-gain)
* [/sys/bus/iio/devices/iio:deviceX/boost\_current\_gain\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-boost-current-gain-available)
* [/sys/bus/iio/devices/iio:deviceX/auto\_zeroing\_mux\_enable](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-auto-zeroing-mux-enable)
* [/sys/bus/iio/devices/iio:deviceX/auto\_zeroing\_ref\_enable](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-auto-zeroing-ref-enable)

## ABI file testing/sysfs-bus-iio-adc-mt6360

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/in\_voltage0\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltage0-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_voltage1\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltage1-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_voltage2\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltage2-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_voltage3\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltage3-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_voltage4\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltage4-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_current5\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-current5-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_current6\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-current6-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_current7\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-current7-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_temp8\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-temp8-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_voltage9\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltage9-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_voltage10\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltage10-raw)

## ABI file testing/sysfs-bus-iio-adc-pac1934

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/in\_shunt\_resistorY](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-shunt-resistory)

## ABI file testing/sysfs-bus-iio-adc-stm32

Has the following ABI:

* [/sys/bus/iio/devices/triggerX/trigger\_polarity](abi-testing.html#abi-sys-bus-iio-devices-triggerx-trigger-polarity)
* [/sys/bus/iio/devices/triggerX/trigger\_polarity\_available](abi-testing.html#abi-sys-bus-iio-devices-triggerx-trigger-polarity-available)

## ABI file testing/sysfs-bus-iio-bno055

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/in\_accel\_raw\_range](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-raw-range)
* [/sys/bus/iio/devices/iio:deviceX/in\_anglvel\_raw\_range](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-anglvel-raw-range)
* [/sys/bus/iio/devices/iio:deviceX/in\_accel\_raw\_range\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-raw-range-available)
* [/sys/bus/iio/devices/iio:deviceX/in\_anglvel\_raw\_range\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-anglvel-raw-range-available)
* [/sys/bus/iio/devices/iio:deviceX/in\_magn\_calibration\_fast\_enable](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-magn-calibration-fast-enable)
* [/sys/bus/iio/devices/iio:deviceX/fusion\_enable](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-fusion-enable)
* [/sys/bus/iio/devices/iio:deviceX/calibration\_data](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-calibration-data)
* [/sys/bus/iio/devices/iio:deviceX/in\_accel\_calibration\_auto\_status](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-calibration-auto-status)
* [/sys/bus/iio/devices/iio:deviceX/in\_gyro\_calibration\_auto\_status](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-gyro-calibration-auto-status)
* [/sys/bus/iio/devices/iio:deviceX/in\_magn\_calibration\_auto\_status](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-magn-calibration-auto-status)
* [/sys/bus/iio/devices/iio:deviceX/sys\_calibration\_auto\_status](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-sys-calibration-auto-status)

## ABI file testing/sysfs-bus-iio-cdc-ad7746

Has the following ABI:

* [/sys/.../iio:deviceX/in\_capacitableY\_calibbias\_calibration](abi-testing.html#abi-sys-iio-devicex-in-capacitabley-calibbias-calibration)
* [/sys/.../iio:deviceX/in\_capacitableY\_calibscale\_calibration](abi-testing.html#abi-sys-iio-devicex-in-capacitabley-calibbias-calibration)

## ABI file testing/sysfs-bus-iio-chemical-sgp40

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/out\_temp\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-temp-raw)
* [/sys/bus/iio/devices/iio:deviceX/out\_humidityrelative\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-humidityrelative-raw)

## ABI file testing/sysfs-bus-iio-chemical-sunrise-co2

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/in\_concentration\_co2\_calibration\_factory](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-concentration-co2-calibration-factory)
* [/sys/bus/iio/devices/iio:deviceX/in\_concentration\_co2\_calibration\_background](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-concentration-co2-calibration-background)
* [/sys/bus/iio/devices/iio:deviceX/error\_status\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-error-status-available)
* [/sys/bus/iio/devices/iio:deviceX/error\_status](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-error-status)

## ABI file testing/sysfs-bus-iio-chemical-vz89x

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/in\_concentration\_VOC\_short\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-concentration-voc-short-raw)

## ABI file testing/sysfs-bus-iio-cros-ec

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/calibrate](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-calibrate)
* [/sys/bus/iio/devices/iio:deviceX/id](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-id)

## ABI file testing/sysfs-bus-iio-dac

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/out\_currentY\_toggle\_en](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-currenty-toggle-en)
* [/sys/bus/iio/devices/iio:deviceX/out\_currentY\_rawN](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-currenty-rawn)
* [/sys/bus/iio/devices/iio:deviceX/out\_currentY\_symbol](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-currenty-symbol)
* [/sys/bus/iio/devices/iio:deviceX/out\_voltageY\_toggle\_en](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-voltagey-toggle-en)
* [/sys/bus/iio/devices/iio:deviceX/out\_voltageY\_rawN](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-voltagey-rawn)
* [/sys/bus/iio/devices/iio:deviceX/out\_voltageY\_symbol](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-voltagey-symbol)

## ABI file testing/sysfs-bus-iio-dac-ad5766

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/in\_voltageY\_dither\_enable](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-dither-enable)
* [/sys/bus/iio/devices/iio:deviceX/in\_voltageY\_dither\_invert](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-dither-invert)
* [/sys/bus/iio/devices/iio:deviceX/in\_voltageY\_dither\_scale\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-dither-scale-available)
* [/sys/bus/iio/devices/iio:deviceX/in\_voltageY\_dither\_scale](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-dither-scale)
* [/sys/bus/iio/devices/iio:deviceX/in\_voltageY\_dither\_source](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltagey-dither-source)

## ABI file testing/sysfs-bus-iio-dac-dpot-dac

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/out\_voltageY\_raw\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-voltagey-raw-available)

## ABI file testing/sysfs-bus-iio-dac-ltc2688

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/out\_voltageY\_dither\_en](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-voltagey-dither-en)
* [/sys/bus/iio/devices/iio:deviceX/out\_voltageY\_dither\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-voltagey-dither-raw)
* [/sys/bus/iio/devices/iio:deviceX/out\_voltageY\_dither\_raw\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-voltagey-dither-raw-available)
* [/sys/bus/iio/devices/iio:deviceX/out\_voltageY\_dither\_offset](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-voltagey-dither-offset)
* [/sys/bus/iio/devices/iio:deviceX/out\_voltageY\_dither\_frequency](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-voltagey-dither-frequency)
* [/sys/bus/iio/devices/iio:deviceX/out\_voltageY\_dither\_frequency\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-voltagey-dither-frequency-available)
* [/sys/bus/iio/devices/iio:deviceX/out\_voltageY\_dither\_phase](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-voltagey-dither-phase)
* [/sys/bus/iio/devices/iio:deviceX/out\_voltageY\_dither\_phase\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-voltagey-dither-phase-available)

## ABI file testing/sysfs-bus-iio-dfsdm-adc-stm32

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/in\_voltage\_spi\_clk\_freq](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-voltage-spi-clk-freq)

## ABI file testing/sysfs-bus-iio-distance-srf08

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/sensor\_max\_range](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-sensor-max-range)

## ABI file testing/sysfs-bus-iio-dma-buffer

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/buffer/length\_align\_bytes](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-buffer-length-align-bytes)

## ABI file testing/sysfs-bus-iio-filter-admv8818

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/filter\_mode\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-filter-mode-available)
* [/sys/bus/iio/devices/iio:deviceX/filter\_mode](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-filter-mode)

## ABI file testing/sysfs-bus-iio-frequency-ad9523

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/pll2\_feedback\_clk\_present](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-pll2-feedback-clk-present)
* [/sys/bus/iio/devices/iio:deviceX/pll2\_reference\_clk\_present](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-pll2-feedback-clk-present)
* [/sys/bus/iio/devices/iio:deviceX/pll1\_reference\_clk\_a\_present](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-pll2-feedback-clk-present)
* [/sys/bus/iio/devices/iio:deviceX/pll1\_reference\_clk\_b\_present](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-pll2-feedback-clk-present)
* [/sys/bus/iio/devices/iio:deviceX/pll1\_reference\_clk\_test\_present](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-pll2-feedback-clk-present)
* [/sys/bus/iio/devices/iio:deviceX/vcxo\_clk\_present](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-pll2-feedback-clk-present)
* [/sys/bus/iio/devices/iio:deviceX/pllY\_locked](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-plly-locked)
* [/sys/bus/iio/devices/iio:deviceX/sync\_dividers](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-sync-dividers)

## ABI file testing/sysfs-bus-iio-frequency-adf4350

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/out\_altvoltageY\_frequency\_resolution](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-altvoltagey-frequency-resolution)
* [/sys/bus/iio/devices/iio:deviceX/out\_altvoltageY\_refin\_frequency](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-altvoltagey-refin-frequency)

## ABI file testing/sysfs-bus-iio-frequency-adf4371

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/out\_altvoltageY\_name](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-altvoltagey-name)

## ABI file testing/sysfs-bus-iio-frequency-admv1013

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/in\_altvoltage0-altvoltage1\_i\_calibphase](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-altvoltage0-altvoltage1-i-calibphase)
* [/sys/bus/iio/devices/iio:deviceX/in\_altvoltage0-altvoltage1\_q\_calibphase](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-altvoltage0-altvoltage1-q-calibphase)
* [/sys/bus/iio/devices/iio:deviceX/in\_altvoltage0\_i\_calibbias](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-altvoltage0-i-calibbias)
* [/sys/bus/iio/devices/iio:deviceX/in\_altvoltage0\_q\_calibbias](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-altvoltage0-q-calibbias)
* [/sys/bus/iio/devices/iio:deviceX/in\_altvoltage1\_i\_calibbias](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-altvoltage1-i-calibbias)
* [/sys/bus/iio/devices/iio:deviceX/in\_altvoltage1\_q\_calibbias](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-altvoltage1-q-calibbias)

## ABI file testing/sysfs-bus-iio-frequency-admv1014

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/in\_altvoltage0\_i\_calibscale\_coarse](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-altvoltage0-i-calibscale-coarse)
* [/sys/bus/iio/devices/iio:deviceX/in\_altvoltage0\_q\_calibscale\_coarse](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-altvoltage0-q-calibscale-coarse)
* [/sys/bus/iio/devices/iio:deviceX/in\_altvoltage0\_i\_calibscale\_fine](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-altvoltage0-i-calibscale-fine)
* [/sys/bus/iio/devices/iio:deviceX/in\_altvoltage0\_q\_calibscale\_fine](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-altvoltage0-q-calibscale-fine)

## ABI file testing/sysfs-bus-iio-gyro-bmg160

Has the following ABI:

* [/sys/bus/iio/devices/triggerX/name = “bmg160-any-motion-devX”](abi-testing.html#abi-sys-bus-iio-devices-triggerx-name-bmg160-any-motion-devx)

## ABI file testing/sysfs-bus-iio-health-afe440x

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/in\_intensityY\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-intensityy-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_intensityY\_offset](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-intensityy-offset)
* [/sys/bus/iio/devices/iio:deviceX/in\_intensityY\_resistance](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-intensityy-resistance)
* [/sys/bus/iio/devices/iio:deviceX/in\_intensityY\_capacitance](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-intensityy-resistance)

## ABI file testing/sysfs-bus-iio-humidity

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/out\_current\_heater\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-current-heater-raw)
* [/sys/bus/iio/devices/iio:deviceX/out\_current\_heater\_raw\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-current-heater-raw)

## ABI file testing/sysfs-bus-iio-impedance-analyzer-ad5933

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/out\_altvoltageY\_frequency\_start](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-altvoltagey-frequency-start)
* [/sys/bus/iio/devices/iio:deviceX/out\_altvoltageY\_frequency\_increment](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-altvoltagey-frequency-increment)
* [/sys/bus/iio/devices/iio:deviceX/out\_altvoltageY\_frequency\_points](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-altvoltagey-frequency-points)
* [/sys/bus/iio/devices/iio:deviceX/out\_altvoltageY\_settling\_cycles](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-altvoltagey-settling-cycles)

## ABI file testing/sysfs-bus-iio-ina2xx-adc

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/in\_allow\_async\_readout](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-allow-async-readout)

## ABI file testing/sysfs-bus-iio-inv\_icm42600

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/in\_accel\_power\_mode](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-power-mode)
* [/sys/bus/iio/devices/iio:deviceX/in\_accel\_power\_mode\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-accel-power-mode-available)

## ABI file testing/sysfs-bus-iio-isl29501

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/in\_proximity0\_agc\_gain](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-proximity0-agc-gain)
* [/sys/bus/iio/devices/iio:deviceX/in\_proximity0\_agc\_gain\_bias](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-proximity0-agc-gain)
* [/sys/bus/iio/devices/iio:deviceX/in\_proximity0\_calib\_phase\_temp\_a](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-proximity0-calib-phase-temp-a)
* [/sys/bus/iio/devices/iio:deviceX/in\_proximity0\_calib\_phase\_temp\_b](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-proximity0-calib-phase-temp-a)
* [/sys/bus/iio/devices/iio:deviceX/in\_proximity0\_calib\_phase\_light\_a](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-proximity0-calib-phase-temp-a)
* [/sys/bus/iio/devices/iio:deviceX/in\_proximity0\_calib\_phase\_light\_b](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-proximity0-calib-phase-temp-a)

## ABI file testing/sysfs-bus-iio-light-isl29018

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/proximity\_on\_chip\_ambient\_infrared\_suppression](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-proximity-on-chip-ambient-infrared-suppression)

## ABI file testing/sysfs-bus-iio-light-lm3533-als

Has the following ABI:

* [/sys/.../events/in\_illuminance0\_thresh\_either\_en](abi-testing.html#abi-sys-events-in-illuminance0-thresh-either-en)
* [/sys/.../events/in\_illuminance0\_threshY\_hysteresis](abi-testing.html#abi-sys-events-in-illuminance0-threshy-hysteresis)
* [/sys/.../events/illuminance\_threshY\_falling\_value](abi-testing.html#abi-sys-events-illuminance-threshy-falling-value)
* [/sys/.../events/illuminance\_threshY\_raising\_value](abi-testing.html#abi-sys-events-illuminance-threshy-falling-value)
* [/sys/bus/iio/devices/iio:deviceX/in\_illuminance0\_zone](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-illuminance0-zone)
* [/sys/bus/iio/devices/iio:deviceX/out\_currentY\_currentZ\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-currenty-currentz-raw)

## ABI file testing/sysfs-bus-iio-light-si1133

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/in\_intensity\_ir\_small\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-intensity-ir-small-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_intensity\_ir\_large\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-intensity-ir-large-raw)
* [/sys/bus/iio/devices/iio:deviceX/in\_intensity\_large\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-intensity-large-raw)

## ABI file testing/sysfs-bus-iio-light-tsl2583

Has the following ABI:

* [/sys/bus/iio/devices/device[n]/in\_illuminance\_calibrate](abi-testing.html#abi-sys-bus-iio-devices-device-n-in-illuminance-calibrate)
* [/sys/bus/iio/devices/device[n]/in\_illuminance\_lux\_table](abi-testing.html#abi-sys-bus-iio-devices-device-n-in-illuminance-lux-table)
* [/sys/bus/iio/devices/device[n]/in\_illuminance\_input\_target](abi-testing.html#abi-sys-bus-iio-devices-device-n-in-illuminance-input-target)

## ABI file testing/sysfs-bus-iio-light-tsl2772

Has the following ABI:

* [/sys/bus/iio/devices/device[n]/in\_illuminance0\_calibrate](abi-testing.html#abi-sys-bus-iio-devices-device-n-in-illuminance0-calibrate)
* [/sys/bus/iio/devices/device[n]/in\_proximity0\_calibrate](abi-testing.html#abi-sys-bus-iio-devices-device-n-in-proximity0-calibrate)

## ABI file testing/sysfs-bus-iio-magnetometer-hmc5843

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/meas\_conf](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-meas-conf)
* [/sys/bus/iio/devices/iio:deviceX/meas\_conf\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-meas-conf)

## ABI file testing/sysfs-bus-iio-meas-spec

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/battery\_low](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-battery-low)

## ABI file testing/sysfs-bus-iio-mpu6050

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/in\_gyro\_matrix](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-gyro-matrix)
* [/sys/bus/iio/devices/iio:deviceX/in\_accel\_matrix](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-gyro-matrix)
* [/sys/bus/iio/devices/iio:deviceX/in\_magn\_matrix](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-gyro-matrix)

## ABI file testing/sysfs-bus-iio-potentiometer-mcp4531

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/out\_resistance\_raw\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-resistance-raw-available)

## ABI file testing/sysfs-bus-iio-proximity

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/in\_proximity\_nearlevel](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-proximity-nearlevel)
* [/sys/bus/iio/devices/iio:deviceX/sensor\_sensitivity](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-sensor-sensitivity)

## ABI file testing/sysfs-bus-iio-proximity-as3935

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/in\_proximity\_input](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-proximity-input)

## ABI file testing/sysfs-bus-iio-resolver-ad2s1210

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/events/in\_altvoltage0\_mag\_rising\_reset\_max](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-events-in-altvoltage0-mag-rising-reset-max)
* [/sys/bus/iio/devices/iio:deviceX/events/in\_altvoltage0\_mag\_rising\_reset\_max\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-events-in-altvoltage0-mag-rising-reset-max-available)
* [/sys/bus/iio/devices/iio:deviceX/events/in\_altvoltage0\_mag\_rising\_reset\_min](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-events-in-altvoltage0-mag-rising-reset-min)
* [/sys/bus/iio/devices/iio:deviceX/events/in\_altvoltage0\_mag\_rising\_reset\_min\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-events-in-altvoltage0-mag-rising-reset-min-available)

## ABI file testing/sysfs-bus-iio-sps30

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/start\_cleaning](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-start-cleaning)
* [/sys/bus/iio/devices/iio:deviceX/cleaning\_period](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-cleaning-period)
* [/sys/bus/iio/devices/iio:deviceX/cleaning\_period\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-cleaning-period-available)

## ABI file testing/sysfs-bus-iio-sx9310

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/in\_proximity3\_comb\_raw](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-proximity3-comb-raw)

## ABI file testing/sysfs-bus-iio-sx9324

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/in\_proximity<id>\_setup](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-proximity-id-setup)

## ABI file testing/sysfs-bus-iio-thermocouple

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/fault\_ovuv](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-fault-ovuv)
* [/sys/bus/iio/devices/iio:deviceX/fault\_oc](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-fault-oc)

## ABI file testing/sysfs-bus-iio-timer-stm32

Has the following ABI:

* [/sys/bus/iio/devices/triggerX/master\_mode\_available](abi-testing.html#abi-sys-bus-iio-devices-triggerx-master-mode-available)
* [/sys/bus/iio/devices/triggerX/master\_mode](abi-testing.html#abi-sys-bus-iio-devices-triggerx-master-mode)
* [/sys/bus/iio/devices/iio:deviceX/in\_count0\_preset](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-count0-preset)
* [/sys/bus/iio/devices/iio:deviceX/in\_count\_enable\_mode\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-count-enable-mode-available)
* [/sys/bus/iio/devices/iio:deviceX/in\_count0\_enable\_mode](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-count0-enable-mode)
* [/sys/bus/iio/devices/iio:deviceX/in\_count\_trigger\_mode\_available](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-count-trigger-mode-available)
* [/sys/bus/iio/devices/iio:deviceX/in\_count0\_trigger\_mode](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-count0-trigger-mode)

## ABI file testing/sysfs-bus-iio-trigger-sysfs

Has the following ABI:

* [/sys/bus/iio/devices/triggerX/trigger\_now](abi-testing.html#abi-sys-bus-iio-devices-triggerx-trigger-now)
* [/sys/bus/iio/devices/triggerX/name](abi-testing.html#abi-sys-bus-iio-devices-triggerx-name)
* [/sys/bus/iio/devices/iio\_sysfs\_trigger/add\_trigger](abi-testing.html#abi-sys-bus-iio-devices-iio-sysfs-trigger-add-trigger)
* [/sys/bus/iio/devices/iio\_sysfs\_trigger/remove\_trigger](abi-testing.html#abi-sys-bus-iio-devices-iio-sysfs-trigger-remove-trigger)

## ABI file testing/sysfs-bus-iio-vf610

Has the following ABI:

* [/sys/bus/iio/devices/iio:deviceX/in\_conversion\_mode](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-in-conversion-mode)
* [/sys/bus/iio/devices/iio:deviceX/out\_conversion\_mode](abi-testing.html#abi-sys-bus-iio-devices-iio-devicex-out-conversion-mode)

## ABI file testing/sysfs-bus-intel\_th-devices-gth

Has the following ABI:

* [/sys/bus/intel\_th/devices/<intel\_th\_id>-gth/masters/\*](abi-testing.html#abi-sys-bus-intel-th-devices-intel-th-id-gth-masters)
* [/sys/bus/intel\_th/devices/<intel\_th\_id>-gth/outputs/[0-7]\_port](abi-testing.html#abi-sys-bus-intel-th-devices-intel-th-id-gth-outputs-0-7-port)
* [/sys/bus/intel\_th/devices/<intel\_th\_id>-gth/outputs/[0-7]\_drop](abi-testing.html#abi-sys-bus-intel-th-devices-intel-th-id-gth-outputs-0-7-drop)
* [/sys/bus/intel\_th/devices/<intel\_th\_id>-gth/outputs/[0-7]\_null](abi-testing.html#abi-sys-bus-intel-th-devices-intel-th-id-gth-outputs-0-7-null)
* [/sys/bus/intel\_th/devices/<intel\_th\_id>-gth/outputs/[0-7]\_flush](abi-testing.html#abi-sys-bus-intel-th-devices-intel-th-id-gth-outputs-0-7-flush)
* [/sys/bus/intel\_th/devices/<intel\_th\_id>-gth/outputs/[0-7]\_reset](abi-testing.html#abi-sys-bus-intel-th-devices-intel-th-id-gth-outputs-0-7-reset)
* [/sys/bus/intel\_th/devices/<intel\_th\_id>-gth/outputs/[0-7]\_smcfreq](abi-testing.html#abi-sys-bus-intel-th-devices-intel-th-id-gth-outputs-0-7-smcfreq)

## ABI file testing/sysfs-bus-intel\_th-devices-msc

Has the following ABI:

* [/sys/bus/intel\_th/devices/<intel\_th\_id>-msc<msc-id>/wrap](abi-testing.html#abi-sys-bus-intel-th-devices-intel-th-id-msc-msc-id-wrap)
* [/sys/bus/intel\_th/devices/<intel\_th\_id>-msc<msc-id>/mode](abi-testing.html#abi-sys-bus-intel-th-devices-intel-th-id-msc-msc-id-mode)
* [/sys/bus/intel\_th/devices/<intel\_th\_id>-msc<msc-id>/nr\_pages](abi-testing.html#abi-sys-bus-intel-th-devices-intel-th-id-msc-msc-id-nr-pages)
* [/sys/bus/intel\_th/devices/<intel\_th\_id>-msc<msc-id>/win\_switch](abi-testing.html#abi-sys-bus-intel-th-devices-intel-th-id-msc-msc-id-win-switch)
* [/sys/bus/intel\_th/devices/<intel\_th\_id>-msc<msc-id>/stop\_on\_full](abi-testing.html#abi-sys-bus-intel-th-devices-intel-th-id-msc-msc-id-stop-on-full)

## ABI file testing/sysfs-bus-intel\_th-devices-pti

Has the following ABI:

* [/sys/bus/intel\_th/devices/<intel\_th\_id>-pti/mode](abi-testing.html#abi-sys-bus-intel-th-devices-intel-th-id-pti-mode)
* [/sys/bus/intel\_th/devices/<intel\_th\_id>-pti/freerunning\_clock](abi-testing.html#abi-sys-bus-intel-th-devices-intel-th-id-pti-freerunning-clock)
* [/sys/bus/intel\_th/devices/<intel\_th\_id>-pti/clock\_divider](abi-testing.html#abi-sys-bus-intel-th-devices-intel-th-id-pti-clock-divider)

## ABI file testing/sysfs-bus-intel\_th-output-devices

Has the following ABI:

* [/sys/bus/intel\_th/devices/<intel\_th\_id>-<device><id>/active](abi-testing.html#abi-sys-bus-intel-th-devices-intel-th-id-device-id-active)
* [/sys/bus/intel\_th/devices/<intel\_th\_id>-msc<msc-id>/port](abi-testing.html#abi-sys-bus-intel-th-devices-intel-th-id-msc-msc-id-port)

## ABI file testing/sysfs-bus-mcb

Has the following ABI:

* [/sys/bus/mcb/devices/mcb:X](abi-testing.html#abi-sys-bus-mcb-devices-mcb-x)
* [/sys/bus/mcb/devices/mcb:X/revision](abi-testing.html#abi-sys-bus-mcb-devices-mcb-x-revision)
* [/sys/bus/mcb/devices/mcb:X/minor](abi-testing.html#abi-sys-bus-mcb-devices-mcb-x-minor)
* [/sys/bus/mcb/devices/mcb:X/model](abi-testing.html#abi-sys-bus-mcb-devices-mcb-x-model)
* [/sys/bus/mcb/devices/mcb:X/name](abi-testing.html#abi-sys-bus-mcb-devices-mcb-x-name)

## ABI file testing/sysfs-bus-mdio

Has the following ABI:

* [/sys/bus/mdio\_bus/devices/.../statistics/](abi-testing.html#abi-sys-bus-mdio-bus-devices-statistics)
* [/sys/class/mdio\_bus/.../statistics/](abi-testing.html#abi-sys-bus-mdio-bus-devices-statistics)
* [/sys/bus/mdio\_bus/devices/.../statistics/transfers](abi-testing.html#abi-sys-bus-mdio-bus-devices-statistics-transfers)
* [/sys/class/mdio\_bus/.../transfers](abi-testing.html#abi-sys-bus-mdio-bus-devices-statistics-transfers)
* [/sys/bus/mdio\_bus/devices/.../statistics/errors](abi-testing.html#abi-sys-bus-mdio-bus-devices-statistics-errors)
* [/sys/class/mdio\_bus/.../statistics/errors](abi-testing.html#abi-sys-bus-mdio-bus-devices-statistics-errors)
* [/sys/bus/mdio\_bus/devices/.../statistics/writes](abi-testing.html#abi-sys-bus-mdio-bus-devices-statistics-writes)
* [/sys/class/mdio\_bus/.../statistics/writes](abi-testing.html#abi-sys-bus-mdio-bus-devices-statistics-writes)
* [/sys/bus/mdio\_bus/devices/.../statistics/reads](abi-testing.html#abi-sys-bus-mdio-bus-devices-statistics-reads)
* [/sys/class/mdio\_bus/.../statistics/reads](abi-testing.html#abi-sys-bus-mdio-bus-devices-statistics-reads)
* [/sys/bus/mdio\_bus/devices/.../statistics/transfers\_<addr>](abi-testing.html#abi-sys-bus-mdio-bus-devices-statistics-transfers-addr)
* [/sys/class/mdio\_bus/.../statistics/transfers\_<addr>](abi-testing.html#abi-sys-bus-mdio-bus-devices-statistics-transfers-addr)
* [/sys/bus/mdio\_bus/devices/.../statistics/errors\_<addr>](abi-testing.html#abi-sys-bus-mdio-bus-devices-statistics-errors-addr)
* [/sys/class/mdio\_bus/.../statistics/errors\_<addr>](abi-testing.html#abi-sys-bus-mdio-bus-devices-statistics-errors-addr)
* [/sys/bus/mdio\_bus/devices/.../statistics/writes\_<addr>](abi-testing.html#abi-sys-bus-mdio-bus-devices-statistics-writes-addr)
* [/sys/class/mdio\_bus/.../statistics/writes\_<addr>](abi-testing.html#abi-sys-bus-mdio-bus-devices-statistics-writes-addr)
* [/sys/bus/mdio\_bus/devices/.../statistics/reads\_<addr>](abi-testing.html#abi-sys-bus-mdio-bus-devices-statistics-reads-addr)
* [/sys/class/mdio\_bus/.../statistics/reads\_<addr>](abi-testing.html#abi-sys-bus-mdio-bus-devices-statistics-reads-addr)

## ABI file testing/sysfs-bus-media

Has the following ABI:

* [/sys/bus/media/devices/.../model](abi-testing.html#abi-sys-bus-media-devices-model)

## ABI file testing/sysfs-bus-mei

Has the following ABI:

* [/sys/bus/mei/devices/.../modalias](abi-testing.html#abi-sys-bus-mei-devices-modalias)
* [/sys/bus/mei/devices/.../name](abi-testing.html#abi-sys-bus-mei-devices-name)
* [/sys/bus/mei/devices/.../uuid](abi-testing.html#abi-sys-bus-mei-devices-uuid)
* [/sys/bus/mei/devices/.../version](abi-testing.html#abi-sys-bus-mei-devices-version)
* [/sys/bus/mei/devices/.../max\_conn](abi-testing.html#abi-sys-bus-mei-devices-max-conn)
* [/sys/bus/mei/devices/.../fixed](abi-testing.html#abi-sys-bus-mei-devices-fixed)
* [/sys/bus/mei/devices/.../vtag](abi-testing.html#abi-sys-bus-mei-devices-vtag)
* [/sys/bus/mei/devices/.../max\_len](abi-testing.html#abi-sys-bus-mei-devices-max-len)

## ABI file testing/sysfs-bus-mmc

Has the following ABI:

* [/sys/bus/mmc/devices/.../rev](abi-testing.html#abi-sys-bus-mmc-devices-rev)

## ABI file testing/sysfs-bus-most

Has the following ABI:

* [/sys/bus/most/devices/<dev>/description](abi-testing.html#abi-sys-bus-most-devices-dev-description)
* [/sys/bus/most/devices/<dev>/interface](abi-testing.html#abi-sys-bus-most-devices-dev-interface)
* [/sys/bus/most/devices/<dev>/dci](abi-testing.html#abi-sys-bus-most-devices-dev-dci)
* [/sys/bus/most/devices/<dev>/dci/arb\_address](abi-testing.html#abi-sys-bus-most-devices-dev-dci-arb-address)
* [/sys/bus/most/devices/<dev>/dci/arb\_value](abi-testing.html#abi-sys-bus-most-devices-dev-dci-arb-value)
* [/sys/bus/most/devices/<dev>/dci/mep\_eui48\_hi](abi-testing.html#abi-sys-bus-most-devices-dev-dci-mep-eui48-hi)
* [/sys/bus/most/devices/<dev>/dci/mep\_eui48\_lo](abi-testing.html#abi-sys-bus-most-devices-dev-dci-mep-eui48-lo)
* [/sys/bus/most/devices/<dev>/dci/mep\_eui48\_mi](abi-testing.html#abi-sys-bus-most-devices-dev-dci-mep-eui48-mi)
* [/sys/bus/most/devices/<dev>/dci/mep\_filter](abi-testing.html#abi-sys-bus-most-devices-dev-dci-mep-filter)
* [/sys/bus/most/devices/<dev>/dci/mep\_hash0](abi-testing.html#abi-sys-bus-most-devices-dev-dci-mep-hash0)
* [/sys/bus/most/devices/<dev>/dci/mep\_hash1](abi-testing.html#abi-sys-bus-most-devices-dev-dci-mep-hash1)
* [/sys/bus/most/devices/<dev>/dci/mep\_hash2](abi-testing.html#abi-sys-bus-most-devices-dev-dci-mep-hash2)
* [/sys/bus/most/devices/<dev>/dci/mep\_hash3](abi-testing.html#abi-sys-bus-most-devices-dev-dci-mep-hash3)
* [/sys/bus/most/devices/<dev>/dci/ni\_state](abi-testing.html#abi-sys-bus-most-devices-dev-dci-ni-state)
* [/sys/bus/most/devices/<dev>/dci/node\_address](abi-testing.html#abi-sys-bus-most-devices-dev-dci-node-address)
* [/sys/bus/most/devices/<dev>/dci/node\_position](abi-testing.html#abi-sys-bus-most-devices-dev-dci-node-position)
* [/sys/bus/most/devices/<dev>/dci/packet\_bandwidth](abi-testing.html#abi-sys-bus-most-devices-dev-dci-packet-bandwidth)
* [/sys/bus/most/devices/<dev>/dci/sync\_ep](abi-testing.html#abi-sys-bus-most-devices-dev-dci-sync-ep)
* [/sys/bus/most/devices/<dev>/<channel>/](abi-testing.html#abi-sys-bus-most-devices-dev-channel)
* [/sys/bus/most/devices/<dev>/<channel>/available\_datatypes](abi-testing.html#abi-sys-bus-most-devices-dev-channel-available-datatypes)
* [/sys/bus/most/devices/<dev>/<channel>/available\_directions](abi-testing.html#abi-sys-bus-most-devices-dev-channel-available-directions)
* [/sys/bus/most/devices/<dev>/<channel>/number\_of\_packet\_buffers](abi-testing.html#abi-sys-bus-most-devices-dev-channel-number-of-packet-buffers)
* [/sys/bus/most/devices/<dev>/<channel>/number\_of\_stream\_buffers](abi-testing.html#abi-sys-bus-most-devices-dev-channel-number-of-stream-buffers)
* [/sys/bus/most/devices/<dev>/<channel>/size\_of\_packet\_buffer](abi-testing.html#abi-sys-bus-most-devices-dev-channel-size-of-packet-buffer)
* [/sys/bus/most/devices/<dev>/<channel>/size\_of\_stream\_buffer](abi-testing.html#abi-sys-bus-most-devices-dev-channel-size-of-stream-buffer)
* [/sys/bus/most/devices/<dev>/<channel>/set\_number\_of\_buffers](abi-testing.html#abi-sys-bus-most-devices-dev-channel-set-number-of-buffers)
* [/sys/bus/most/devices/<dev>/<channel>/set\_buffer\_size](abi-testing.html#abi-sys-bus-most-devices-dev-channel-set-buffer-size)
* [/sys/bus/most/devices/<dev>/<channel>/set\_direction](abi-testing.html#abi-sys-bus-most-devices-dev-channel-set-direction)
* [/sys/bus/most/devices/<dev>/<channel>/set\_datatype](abi-testing.html#abi-sys-bus-most-devices-dev-channel-set-datatype)
* [/sys/bus/most/devices/<dev>/<channel>/set\_subbuffer\_size](abi-testing.html#abi-sys-bus-most-devices-dev-channel-set-subbuffer-size)
* [/sys/bus/most/devices/<dev>/<channel>/set\_packets\_per\_xact](abi-testing.html#abi-sys-bus-most-devices-dev-channel-set-packets-per-xact)
* [/sys/bus/most/devices/<dev>/<channel>/channel\_starving](abi-testing.html#abi-sys-bus-most-devices-dev-channel-channel-starving)
* [/sys/bus/most/drivers/most\_core/components](abi-testing.html#abi-sys-bus-most-drivers-most-core-components)
* [/sys/bus/most/drivers/most\_core/links](abi-testing.html#abi-sys-bus-most-drivers-most-core-links)

## ABI file testing/sysfs-bus-moxtet-devices

Has the following ABI:

* [/sys/bus/moxtet/devices/moxtet-<name>.<addr>/module\_description](abi-testing.html#abi-sys-bus-moxtet-devices-moxtet-name-addr-module-description)
* [/sys/bus/moxtet/devices/moxtet-<name>.<addr>/module\_id](abi-testing.html#abi-sys-bus-moxtet-devices-moxtet-name-addr-module-id)
* [/sys/bus/moxtet/devices/moxtet-<name>.<addr>/module\_name](abi-testing.html#abi-sys-bus-moxtet-devices-moxtet-name-addr-module-name)

## ABI file testing/sysfs-bus-nfit

For all of the nmem device attributes under `nfit/*`, see the ‘NVDIMM Firmware
Interface Table (NFIT)’ section in the ACPI specification
(<http://www.uefi.org/specifications>) for more details.

Has the following ABI:

* [/sys/bus/nd/devices/nmemX/nfit/serial](abi-testing.html#abi-sys-bus-nd-devices-nmemx-nfit-serial)
* [/sys/bus/nd/devices/nmemX/nfit/handle](abi-testing.html#abi-sys-bus-nd-devices-nmemx-nfit-handle)
* [/sys/bus/nd/devices/nmemX/nfit/device](abi-testing.html#abi-sys-bus-nd-devices-nmemx-nfit-device)
* [/sys/bus/nd/devices/nmemX/nfit/rev\_id](abi-testing.html#abi-sys-bus-nd-devices-nmemx-nfit-rev-id)
* [/sys/bus/nd/devices/nmemX/nfit/phys\_id](abi-testing.html#abi-sys-bus-nd-devices-nmemx-nfit-phys-id)
* [/sys/bus/nd/devices/nmemX/nfit/flags](abi-testing.html#abi-sys-bus-nd-devices-nmemx-nfit-flags)
* [/sys/bus/nd/devices/nmemX/nfit/format](abi-testing.html#abi-sys-bus-nd-devices-nmemx-nfit-format)
* [/sys/bus/nd/devices/nmemX/nfit/format1](abi-testing.html#abi-sys-bus-nd-devices-nmemx-nfit-format)
* [/sys/bus/nd/devices/nmemX/nfit/formats](abi-testing.html#abi-sys-bus-nd-devices-nmemx-nfit-format)
* [/sys/bus/nd/devices/nmemX/nfit/vendor](abi-testing.html#abi-sys-bus-nd-devices-nmemx-nfit-vendor)
* [/sys/bus/nd/devices/nmemX/nfit/dsm\_mask](abi-testing.html#abi-sys-bus-nd-devices-nmemx-nfit-dsm-mask)
* [/sys/bus/nd/devices/nmemX/nfit/family](abi-testing.html#abi-sys-bus-nd-devices-nmemx-nfit-family)
* [/sys/bus/nd/devices/nmemX/nfit/id](abi-testing.html#abi-sys-bus-nd-devices-nmemx-nfit-id)
* [/sys/bus/nd/devices/nmemX/nfit/subsystem\_vendor](abi-testing.html#abi-sys-bus-nd-devices-nmemx-nfit-subsystem-vendor)
* [/sys/bus/nd/devices/nmemX/nfit/subsystem\_rev\_id](abi-testing.html#abi-sys-bus-nd-devices-nmemx-nfit-subsystem-rev-id)
* [/sys/bus/nd/devices/nmemX/nfit/subsystem\_device](abi-testing.html#abi-sys-bus-nd-devices-nmemx-nfit-subsystem-device)
* [/sys/bus/nd/devices/ndbusX/nfit/revision](abi-testing.html#abi-sys-bus-nd-devices-ndbusx-nfit-revision)
* [/sys/bus/nd/devices/ndbusX/nfit/scrub](abi-testing.html#abi-sys-bus-nd-devices-ndbusx-nfit-scrub)
* [/sys/bus/nd/devices/ndbusX/nfit/hw\_error\_scrub](abi-testing.html#abi-sys-bus-nd-devices-ndbusx-nfit-hw-error-scrub)
* [/sys/bus/nd/devices/ndbusX/nfit/dsm\_mask](abi-testing.html#abi-sys-bus-nd-devices-ndbusx-nfit-dsm-mask)
* [/sys/bus/nd/devices/ndbusX/nfit/firmware\_activate\_noidle](abi-testing.html#abi-sys-bus-nd-devices-ndbusx-nfit-firmware-activate-noidle)
* [/sys/bus/nd/devices/regionX/nfit/range\_index](abi-testing.html#abi-sys-bus-nd-devices-regionx-nfit-range-index)

## ABI file testing/sysfs-bus-nvdimm

Has the following ABI:

* [nvdimm](abi-testing.html#abi-nvdimm)
* [/sys/bus/event\_source/devices/nmemX/format](abi-testing.html#abi-sys-bus-event-source-devices-nmemx-format)
* [/sys/bus/event\_source/devices/nmemX/events](abi-testing.html#abi-sys-bus-event-source-devices-nmemx-events)
* [/sys/bus/event\_source/devices/nmemX/cpumask](abi-testing.html#abi-sys-bus-event-source-devices-nmemx-cpumask)
* [/sys/bus/nd/devices/nmemX/cxl/id](abi-testing.html#abi-sys-bus-nd-devices-nmemx-cxl-id)
* [/sys/bus/nd/devices/nmemX/cxl/provider](abi-testing.html#abi-sys-bus-nd-devices-nmemx-cxl-provider)

## ABI file testing/sysfs-bus-optee-devices

Has the following ABI:

* [/sys/bus/tee/devices/optee-ta-<uuid>/](abi-testing.html#abi-sys-bus-tee-devices-optee-ta-uuid)
* [/sys/bus/tee/devices/optee-ta-<uuid>/need\_supplicant](abi-testing.html#abi-sys-bus-tee-devices-optee-ta-uuid-need-supplicant)

## ABI file testing/sysfs-bus-papr-pmem

Has the following ABI:

* [/sys/bus/nd/devices/nmemX/papr/flags](abi-testing.html#abi-sys-bus-nd-devices-nmemx-papr-flags)
* [/sys/bus/nd/devices/nmemX/papr/perf\_stats](abi-testing.html#abi-sys-bus-nd-devices-nmemx-papr-perf-stats)
* [/sys/bus/nd/devices/nmemX/papr/health\_bitmap\_inject](abi-testing.html#abi-sys-bus-nd-devices-nmemx-papr-health-bitmap-inject)

## ABI file testing/sysfs-bus-pci

Has the following ABI:

* [/sys/bus/pci/drivers/.../bind](abi-testing.html#abi-sys-bus-pci-drivers-bind)
* [/sys/devices/pciX/.../bind](abi-testing.html#abi-sys-bus-pci-drivers-bind)
* [/sys/bus/pci/drivers/.../unbind](abi-testing.html#abi-sys-bus-pci-drivers-unbind)
* [/sys/devices/pciX/.../unbind](abi-testing.html#abi-sys-bus-pci-drivers-unbind)
* [/sys/bus/pci/drivers/.../new\_id](abi-testing.html#abi-sys-bus-pci-drivers-new-id)
* [/sys/devices/pciX/.../new\_id](abi-testing.html#abi-sys-bus-pci-drivers-new-id)
* [/sys/bus/pci/drivers/.../remove\_id](abi-testing.html#abi-sys-bus-pci-drivers-remove-id)
* [/sys/devices/pciX/.../remove\_id](abi-testing.html#abi-sys-bus-pci-drivers-remove-id)
* [/sys/bus/pci/rescan](abi-testing.html#abi-sys-bus-pci-rescan)
* [/sys/bus/pci/devices/.../msi\_bus](abi-testing.html#abi-sys-bus-pci-devices-msi-bus)
* [/sys/bus/pci/devices/.../msi\_irqs/](abi-testing.html#abi-sys-bus-pci-devices-msi-irqs)
* [/sys/bus/pci/devices/.../msi\_irqs/<N>](abi-testing.html#abi-sys-bus-pci-devices-msi-irqs-n)
* [/sys/bus/pci/devices/.../irq](abi-testing.html#abi-sys-bus-pci-devices-irq)
* [/sys/bus/pci/devices/.../remove](abi-testing.html#abi-sys-bus-pci-devices-remove)
* [/sys/bus/pci/devices/.../pci\_bus/.../rescan](abi-testing.html#abi-sys-bus-pci-devices-pci-bus-rescan)
* [/sys/bus/pci/devices/.../rescan](abi-testing.html#abi-sys-bus-pci-devices-rescan)
* [/sys/bus/pci/devices/.../reset\_method](abi-testing.html#abi-sys-bus-pci-devices-reset-method)
* [/sys/bus/pci/devices/.../reset](abi-testing.html#abi-sys-bus-pci-devices-reset)
* [/sys/bus/pci/devices/.../reset\_subordinate](abi-testing.html#abi-sys-bus-pci-devices-reset-subordinate)
* [/sys/bus/pci/devices/.../vpd](abi-testing.html#abi-sys-bus-pci-devices-vpd)
* [/sys/bus/pci/devices/.../virtfn<N>](abi-testing.html#abi-sys-bus-pci-devices-virtfn-n)
* [/sys/bus/pci/devices/.../dep\_link](abi-testing.html#abi-sys-bus-pci-devices-dep-link)
* [/sys/bus/pci/devices/.../physfn](abi-testing.html#abi-sys-bus-pci-devices-physfn)
* [/sys/bus/pci/devices/.../modalias](abi-testing.html#abi-sys-bus-pci-devices-modalias)
* [/sys/bus/pci/slots/.../module](abi-testing.html#abi-sys-bus-pci-slots-module)
* [/sys/bus/pci/devices/.../label](abi-testing.html#abi-sys-bus-pci-devices-label)
* [/sys/bus/pci/devices/.../index](abi-testing.html#abi-sys-bus-pci-devices-index)
* [/sys/bus/pci/devices/.../acpi\_index](abi-testing.html#abi-sys-bus-pci-devices-acpi-index)
* [/sys/bus/pci/devices/.../d3cold\_allowed](abi-testing.html#abi-sys-bus-pci-devices-d3cold-allowed)
* [/sys/bus/pci/devices/.../sriov\_totalvfs](abi-testing.html#abi-sys-bus-pci-devices-sriov-totalvfs)
* [/sys/bus/pci/devices/.../sriov\_numvfs](abi-testing.html#abi-sys-bus-pci-devices-sriov-numvfs)
* [/sys/bus/pci/devices/.../driver\_override](abi-testing.html#abi-sys-bus-pci-devices-driver-override)
* [/sys/bus/pci/devices/.../numa\_node](abi-testing.html#abi-sys-bus-pci-devices-numa-node)
* [/sys/bus/pci/devices/.../revision](abi-testing.html#abi-sys-bus-pci-devices-revision)
* [/sys/bus/pci/devices/.../sriov\_drivers\_autoprobe](abi-testing.html#abi-sys-bus-pci-devices-sriov-drivers-autoprobe)
* [/sys/bus/pci/devices/.../p2pmem/size](abi-testing.html#abi-sys-bus-pci-devices-p2pmem-size)
* [/sys/bus/pci/devices/.../p2pmem/available](abi-testing.html#abi-sys-bus-pci-devices-p2pmem-available)
* [/sys/bus/pci/devices/.../p2pmem/published](abi-testing.html#abi-sys-bus-pci-devices-p2pmem-published)
* [/sys/bus/pci/devices/.../p2pmem/allocate](abi-testing.html#abi-sys-bus-pci-devices-p2pmem-allocate)
* [/sys/bus/pci/devices/.../link/clkpm](abi-testing.html#abi-sys-bus-pci-devices-link-clkpm)
* [/sys/bus/pci/devices/.../power\_state](abi-testing.html#abi-sys-bus-pci-devices-power-state)
* [/sys/bus/pci/devices/.../sriov\_vf\_total\_msix](abi-testing.html#abi-sys-bus-pci-devices-sriov-vf-total-msix)
* [/sys/bus/pci/devices/.../sriov\_vf\_msix\_count](abi-testing.html#abi-sys-bus-pci-devices-sriov-vf-msix-count)
* [/sys/bus/pci/devices/.../resourceN\_resize](abi-testing.html#abi-sys-bus-pci-devices-resourcen-resize)
* [/sys/bus/pci/devices/.../leds/\*:enclosure:\*/brightness](abi-testing.html#abi-sys-bus-pci-devices-leds-enclosure-brightness)
* [/sys/class/leds/\*:enclosure:\*/brightness](abi-testing.html#abi-sys-bus-pci-devices-leds-enclosure-brightness)
* [/sys/bus/pci/devices/.../doe\_features](abi-testing.html#abi-sys-bus-pci-devices-doe-features)
* [/sys/bus/pci/devices/.../serial\_number](abi-testing.html#abi-sys-bus-pci-devices-serial-number)
* [/sys/bus/pci/devices/.../tsm/](abi-testing.html#abi-sys-bus-pci-devices-tsm)
* [/sys/bus/pci/devices/.../tsm/connect](abi-testing.html#abi-sys-bus-pci-devices-tsm-connect)
* [/sys/bus/pci/devices/.../tsm/disconnect](abi-testing.html#abi-sys-bus-pci-devices-tsm-disconnect)
* [/sys/bus/pci/devices/.../tsm/dsm](abi-testing.html#abi-sys-bus-pci-devices-tsm-dsm)
* [/sys/bus/pci/devices/.../tsm/bound](abi-testing.html#abi-sys-bus-pci-devices-tsm-bound)
* [/sys/bus/pci/devices/.../authenticated](abi-testing.html#abi-sys-bus-pci-devices-authenticated)

## ABI file testing/sysfs-bus-pci-devices-aer

PCIe Device AER statistics

These attributes show up under all the devices that are AER capable. These
statistical counters indicate the errors “as seen/reported by the device”.
Note that this may mean that if an endpoint is causing problems, the AER
counters may increment at its link partner (e.g. root port) because the
errors may be “seen” / reported by the link partner and not the
problematic endpoint itself (which may report all counters as 0 as it never
saw any problems).

Has the following ABI:

* [/sys/bus/pci/devices/<dev>/aer\_dev\_correctable](abi-testing.html#abi-sys-bus-pci-devices-dev-aer-dev-correctable)
* [/sys/bus/pci/devices/<dev>/aer\_dev\_fatal](abi-testing.html#abi-sys-bus-pci-devices-dev-aer-dev-fatal)
* [/sys/bus/pci/devices/<dev>/aer\_dev\_nonfatal](abi-testing.html#abi-sys-bus-pci-devices-dev-aer-dev-nonfatal)
* [/sys/bus/pci/devices/<dev>/aer\_rootport\_total\_err\_cor](abi-testing.html#abi-sys-bus-pci-devices-dev-aer-rootport-total-err-cor)
* [/sys/bus/pci/devices/<dev>/aer\_rootport\_total\_err\_fatal](abi-testing.html#abi-sys-bus-pci-devices-dev-aer-rootport-total-err-fatal)
* [/sys/bus/pci/devices/<dev>/aer\_rootport\_total\_err\_nonfatal](abi-testing.html#abi-sys-bus-pci-devices-dev-aer-rootport-total-err-nonfatal)
* [/sys/bus/pci/devices/<dev>/aer/correctable\_ratelimit\_interval\_ms](abi-testing.html#abi-sys-bus-pci-devices-dev-aer-correctable-ratelimit-interval-ms)
* [/sys/bus/pci/devices/<dev>/aer/correctable\_ratelimit\_burst](abi-testing.html#abi-sys-bus-pci-devices-dev-aer-correctable-ratelimit-burst)
* [/sys/bus/pci/devices/<dev>/aer/nonfatal\_ratelimit\_interval\_ms](abi-testing.html#abi-sys-bus-pci-devices-dev-aer-nonfatal-ratelimit-interval-ms)
* [/sys/bus/pci/devices/<dev>/aer/nonfatal\_ratelimit\_burst](abi-testing.html#abi-sys-bus-pci-devices-dev-aer-nonfatal-ratelimit-burst)

## ABI file testing/sysfs-bus-pci-devices-avs

Has the following ABI:

* [/sys/devices/pci0000:00/<dev>/avs/fw\_version](abi-testing.html#abi-sys-devices-pci0000-00-dev-avs-fw-version)

## ABI file testing/sysfs-bus-pci-devices-catpt

Has the following ABI:

* [/sys/devices/pci0000:00/<dev>/fw\_version](abi-testing.html#abi-sys-devices-pci0000-00-dev-fw-version)
* [/sys/devices/pci0000:00/<dev>/fw\_info](abi-testing.html#abi-sys-devices-pci0000-00-dev-fw-info)

## ABI file testing/sysfs-bus-pci-devices-cciss

Has the following ABI:

* [/sys/bus/pci/devices/<dev>/ccissX/cXdY/model](abi-testing.html#abi-sys-bus-pci-devices-dev-ccissx-cxdy-model)
* [/sys/bus/pci/devices/<dev>/ccissX/cXdY/rev](abi-testing.html#abi-sys-bus-pci-devices-dev-ccissx-cxdy-rev)
* [/sys/bus/pci/devices/<dev>/ccissX/cXdY/unique\_id](abi-testing.html#abi-sys-bus-pci-devices-dev-ccissx-cxdy-unique-id)
* [/sys/bus/pci/devices/<dev>/ccissX/cXdY/vendor](abi-testing.html#abi-sys-bus-pci-devices-dev-ccissx-cxdy-vendor)
* [/sys/bus/pci/devices/<dev>/ccissX/cXdY/block:cciss!cXdY](abi-testing.html#abi-sys-bus-pci-devices-dev-ccissx-cxdy-block-cciss-cxdy)
* [/sys/bus/pci/devices/<dev>/ccissX/rescan](abi-testing.html#abi-sys-bus-pci-devices-dev-ccissx-rescan)
* [/sys/bus/pci/devices/<dev>/ccissX/cXdY/lunid](abi-testing.html#abi-sys-bus-pci-devices-dev-ccissx-cxdy-lunid)
* [/sys/bus/pci/devices/<dev>/ccissX/cXdY/raid\_level](abi-testing.html#abi-sys-bus-pci-devices-dev-ccissx-cxdy-raid-level)
* [/sys/bus/pci/devices/<dev>/ccissX/cXdY/usage\_count](abi-testing.html#abi-sys-bus-pci-devices-dev-ccissx-cxdy-usage-count)
* [/sys/bus/pci/devices/<dev>/ccissX/resettable](abi-testing.html#abi-sys-bus-pci-devices-dev-ccissx-resettable)
* [/sys/bus/pci/devices/<dev>/ccissX/transport\_mode](abi-testing.html#abi-sys-bus-pci-devices-dev-ccissx-transport-mode)

## ABI file testing/sysfs-bus-pci-devices-pvpanic

Has the following ABI:

* [/sys/devices/pci0000:00/\*/QEMU0001:00/capability for MMIO](abi-testing.html#abi-sys-devices-pci0000-00-qemu0001-00-capability-for-mmio)
* [/sys/devices/pci0000:00/\*/QEMU0001:00/events](abi-testing.html#abi-sys-devices-pci0000-00-qemu0001-00-events)

## ABI file testing/sysfs-bus-pci-drivers-ehci\_hcd

Has the following ABI:

* [/sys/bus/pci/drivers/ehci\_hcd/.../companion](abi-testing.html#abi-sys-bus-pci-drivers-ehci-hcd-companion)

## ABI file testing/sysfs-bus-pci-drivers-janz-cmodio

Has the following ABI:

* [/sys/bus/pci/drivers/janz-cmodio/.../modulbus\_number](abi-testing.html#abi-sys-bus-pci-drivers-janz-cmodio-modulbus-number)

## ABI file testing/sysfs-bus-pci-drivers-xhci\_hcd

Has the following ABI:

* [/sys/bus/pci/drivers/xhci\_hcd/.../dbc](abi-testing.html#abi-sys-bus-pci-drivers-xhci-hcd-dbc)
* [/sys/bus/pci/drivers/xhci\_hcd/.../dbc\_idVendor](abi-testing.html#abi-sys-bus-pci-drivers-xhci-hcd-dbc-idvendor)
* [/sys/bus/pci/drivers/xhci\_hcd/.../dbc\_idProduct](abi-testing.html#abi-sys-bus-pci-drivers-xhci-hcd-dbc-idproduct)
* [/sys/bus/pci/drivers/xhci\_hcd/.../dbc\_bcdDevice](abi-testing.html#abi-sys-bus-pci-drivers-xhci-hcd-dbc-bcddevice)
* [/sys/bus/pci/drivers/xhci\_hcd/.../dbc\_bInterfaceProtocol](abi-testing.html#abi-sys-bus-pci-drivers-xhci-hcd-dbc-binterfaceprotocol)
* [/sys/bus/pci/drivers/xhci\_hcd/.../dbc\_poll\_interval\_ms](abi-testing.html#abi-sys-bus-pci-drivers-xhci-hcd-dbc-poll-interval-ms)
* [/sys/bus/pci/drivers/xhci\_hcd/.../dbc\_serial](abi-testing.html#abi-sys-bus-pci-drivers-xhci-hcd-dbc-serial)
* [/sys/bus/pci/drivers/xhci\_hcd/.../dbc\_product](abi-testing.html#abi-sys-bus-pci-drivers-xhci-hcd-dbc-product)
* [/sys/bus/pci/drivers/xhci\_hcd/.../dbc\_manufacturer](abi-testing.html#abi-sys-bus-pci-drivers-xhci-hcd-dbc-manufacturer)

## ABI file testing/sysfs-bus-peci

Has the following ABI:

* [/sys/bus/peci/rescan](abi-testing.html#abi-sys-bus-peci-rescan)
* [/sys/bus/peci/devices/<controller\_id>-<device\_addr>/remove](abi-testing.html#abi-sys-bus-peci-devices-controller-id-device-addr-remove)

## ABI file testing/sysfs-bus-platform

Has the following ABI:

* [/sys/bus/platform/devices/.../driver\_override](abi-testing.html#abi-sys-bus-platform-devices-driver-override)
* [/sys/bus/platform/devices/.../numa\_node](abi-testing.html#abi-sys-bus-platform-devices-numa-node)
* [/sys/bus/platform/devices/.../msi\_irqs/](abi-testing.html#abi-sys-bus-platform-devices-msi-irqs)
* [/sys/bus/platform/devices/.../msi\_irqs/<N>](abi-testing.html#abi-sys-bus-platform-devices-msi-irqs-n)
* [/sys/bus/platform/devices/.../modalias](abi-testing.html#abi-sys-bus-platform-devices-modalias)

## ABI file testing/sysfs-bus-platform-devices-ampere-smpro

Has the following ABI:

* [/sys/bus/platform/devices/smpro-errmon.\*/error\_[core|mem|pcie|other]\_[ce|ue]](abi-testing.html#abi-sys-bus-platform-devices-smpro-errmon-error-core-mem-pcie-other-ce-ue)
* [/sys/bus/platform/devices/smpro-errmon.\*/overflow\_[core|mem|pcie|other]\_[ce|ue]](abi-testing.html#abi-sys-bus-platform-devices-smpro-errmon-overflow-core-mem-pcie-other-ce-ue)
* [/sys/bus/platform/devices/smpro-errmon.\*/[error|warn]\_[smpro|pmpro]](abi-testing.html#abi-sys-bus-platform-devices-smpro-errmon-error-warn-smpro-pmpro)
* [/sys/bus/platform/devices/smpro-errmon.\*/event\_[vrd\_warn\_fault|vrd\_hot|dimm\_hot|dimm\_2x\_refresh]](abi-testing.html#abi-sys-bus-platform-devices-smpro-errmon-event-vrd-warn-fault-vrd-hot-dimm-hot-dimm-2x-refresh)
* [/sys/bus/platform/devices/smpro-errmon.\*/event\_dimm[0-15]\_syndrome](abi-testing.html#abi-sys-bus-platform-devices-smpro-errmon-event-dimm-0-15-syndrome)
* [/sys/bus/platform/devices/smpro-misc.\*/boot\_progress](abi-testing.html#abi-sys-bus-platform-devices-smpro-misc-boot-progress)
* [/sys/bus/platform/devices/smpro-misc\*/soc\_power\_limit](abi-testing.html#abi-sys-bus-platform-devices-smpro-misc-soc-power-limit)

## ABI file testing/sysfs-bus-platform-devices-occ-hwmon

Has the following ABI:

* [/sys/bus/platform/devices/occ-hwmon.X/ffdc](abi-testing.html#abi-sys-bus-platform-devices-occ-hwmon-x-ffdc)

## ABI file testing/sysfs-bus-platform-drivers-amd\_x3d\_vcache

Has the following ABI:

* [/sys/bus/platform/drivers/amd\_x3d\_vcache/AMDI0101:00/amd\_x3d\_mode](abi-testing.html#abi-sys-bus-platform-drivers-amd-x3d-vcache-amdi0101-00-amd-x3d-mode)

## ABI file testing/sysfs-bus-platform-onboard-usb-dev

Has the following ABI:

* [/sys/bus/platform/devices/<dev>/always\_powered\_in\_suspend](abi-testing.html#abi-sys-bus-platform-devices-dev-always-powered-in-suspend)

## ABI file testing/sysfs-bus-rapidio

Has the following ABI:

* [/sys/bus/rapidio/devices/<nn>:<d>:<iiii>](abi-testing.html#abi-sys-bus-rapidio-devices-nn-d-iiii)
* [/sys/bus/rapidio/devices/<nn>:<d>:<iiii>/did](abi-testing.html#abi-sys-bus-rapidio-devices-nn-d-iiii-did)
* [/sys/bus/rapidio/devices/<nn>:<d>:<iiii>/vid](abi-testing.html#abi-sys-bus-rapidio-devices-nn-d-iiii-vid)
* [/sys/bus/rapidio/devices/<nn>:<d>:<iiii>/device\_rev](abi-testing.html#abi-sys-bus-rapidio-devices-nn-d-iiii-device-rev)
* [/sys/bus/rapidio/devices/<nn>:<d>:<iiii>/asm\_did](abi-testing.html#abi-sys-bus-rapidio-devices-nn-d-iiii-asm-did)
* [/sys/bus/rapidio/devices/<nn>:<d>:<iiii>/asm\_rev](abi-testing.html#abi-sys-bus-rapidio-devices-nn-d-iiii-asm-rev)
* [/sys/bus/rapidio/devices/<nn>:<d>:<iiii>/asm\_vid](abi-testing.html#abi-sys-bus-rapidio-devices-nn-d-iiii-asm-vid)
* [/sys/bus/rapidio/devices/<nn>:<d>:<iiii>/destid](abi-testing.html#abi-sys-bus-rapidio-devices-nn-d-iiii-destid)
* [/sys/bus/rapidio/devices/<nn>:<d>:<iiii>/lprev](abi-testing.html#abi-sys-bus-rapidio-devices-nn-d-iiii-lprev)
* [/sys/bus/rapidio/devices/<nn>:<d>:<iiii>/modalias](abi-testing.html#abi-sys-bus-rapidio-devices-nn-d-iiii-modalias)
* [/sys/bus/rapidio/devices/<nn>:<d>:<iiii>/config](abi-testing.html#abi-sys-bus-rapidio-devices-nn-d-iiii-config)
* [/sys/bus/rapidio/devices/<nn>:<s>:<iiii>/routes](abi-testing.html#abi-sys-bus-rapidio-devices-nn-s-iiii-routes)
* [/sys/bus/rapidio/devices/<nn>:<s>:<iiii>/destid](abi-testing.html#abi-sys-bus-rapidio-devices-nn-s-iiii-destid)
* [/sys/bus/rapidio/devices/<nn>:<s>:<iiii>/hopcount](abi-testing.html#abi-sys-bus-rapidio-devices-nn-s-iiii-hopcount)
* [/sys/bus/rapidio/devices/<nn>:<s>:<iiii>/lnext](abi-testing.html#abi-sys-bus-rapidio-devices-nn-s-iiii-lnext)
* [/sys/bus/rapidio/devices/<nn>:<s>:<iiii>/errlog](abi-testing.html#abi-sys-bus-rapidio-devices-nn-s-iiii-errlog)
* [/sys/bus/rapidio/scan](abi-testing.html#abi-sys-bus-rapidio-scan)

## ABI file testing/sysfs-bus-rbd

Has the following ABI:

* [/sys/bus/rbd/add](abi-testing.html#abi-sys-bus-rbd-add)
* [/sys/bus/rbd/remove](abi-testing.html#abi-sys-bus-rbd-remove)
* [/sys/bus/rbd/add\_single\_major](abi-testing.html#abi-sys-bus-rbd-add-single-major)
* [/sys/bus/rbd/remove\_single\_major](abi-testing.html#abi-sys-bus-rbd-remove-single-major)
* [/sys/bus/rbd/supported\_features](abi-testing.html#abi-sys-bus-rbd-supported-features)
* [/sys/bus/rbd/devices/<dev-id>/size](abi-testing.html#abi-sys-bus-rbd-devices-dev-id-size)
* [/sys/bus/rbd/devices/<dev-id>/major](abi-testing.html#abi-sys-bus-rbd-devices-dev-id-size)
* [/sys/bus/rbd/devices/<dev-id>/client\_id](abi-testing.html#abi-sys-bus-rbd-devices-dev-id-size)
* [/sys/bus/rbd/devices/<dev-id>/pool](abi-testing.html#abi-sys-bus-rbd-devices-dev-id-size)
* [/sys/bus/rbd/devices/<dev-id>/name](abi-testing.html#abi-sys-bus-rbd-devices-dev-id-size)
* [/sys/bus/rbd/devices/<dev-id>/refresh](abi-testing.html#abi-sys-bus-rbd-devices-dev-id-size)
* [/sys/bus/rbd/devices/<dev-id>/current\_snap](abi-testing.html#abi-sys-bus-rbd-devices-dev-id-size)
* [/sys/bus/rbd/devices/<dev-id>/pool\_id](abi-testing.html#abi-sys-bus-rbd-devices-dev-id-pool-id)
* [/sys/bus/rbd/devices/<dev-id>/image\_id](abi-testing.html#abi-sys-bus-rbd-devices-dev-id-image-id)
* [/sys/bus/rbd/devices/<dev-id>/features](abi-testing.html#abi-sys-bus-rbd-devices-dev-id-image-id)
* [/sys/bus/rbd/devices/<dev-id>/parent](abi-testing.html#abi-sys-bus-rbd-devices-dev-id-parent)
* [/sys/bus/rbd/devices/<dev-id>/minor](abi-testing.html#abi-sys-bus-rbd-devices-dev-id-minor)
* [/sys/bus/rbd/devices/<dev-id>/snap\_id](abi-testing.html#abi-sys-bus-rbd-devices-dev-id-snap-id)
* [/sys/bus/rbd/devices/<dev-id>/config\_info](abi-testing.html#abi-sys-bus-rbd-devices-dev-id-snap-id)
* [/sys/bus/rbd/devices/<dev-id>/cluster\_fsid](abi-testing.html#abi-sys-bus-rbd-devices-dev-id-snap-id)
* [/sys/bus/rbd/devices/<dev-id>/client\_addr](abi-testing.html#abi-sys-bus-rbd-devices-dev-id-snap-id)

## ABI file testing/sysfs-bus-rpmsg

Has the following ABI:

* [/sys/bus/rpmsg/devices/.../name](abi-testing.html#abi-sys-bus-rpmsg-devices-name)
* [/sys/bus/rpmsg/devices/.../src](abi-testing.html#abi-sys-bus-rpmsg-devices-src)
* [/sys/bus/rpmsg/devices/.../dst](abi-testing.html#abi-sys-bus-rpmsg-devices-dst)
* [/sys/bus/rpmsg/devices/.../announce](abi-testing.html#abi-sys-bus-rpmsg-devices-announce)
* [/sys/bus/rpmsg/devices/.../driver\_override](abi-testing.html#abi-sys-bus-rpmsg-devices-driver-override)

## ABI file testing/sysfs-bus-siox

Has the following ABI:

* [/sys/bus/siox/devices/siox-X/active](abi-testing.html#abi-sys-bus-siox-devices-siox-x-active)
* [/sys/bus/siox/devices/siox-X/device\_add](abi-testing.html#abi-sys-bus-siox-devices-siox-x-device-add)
* [/sys/bus/siox/devices/siox-X/device\_remove](abi-testing.html#abi-sys-bus-siox-devices-siox-x-device-remove)
* [/sys/bus/siox/devices/siox-X/poll\_interval\_ns](abi-testing.html#abi-sys-bus-siox-devices-siox-x-poll-interval-ns)
* [/sys/bus/siox/devices/siox-X-Y/connected](abi-testing.html#abi-sys-bus-siox-devices-siox-x-y-connected)
* [/sys/bus/siox/devices/siox-X-Y/inbytes](abi-testing.html#abi-sys-bus-siox-devices-siox-x-y-inbytes)
* [/sys/bus/siox/devices/siox-X-Y/status\_errors](abi-testing.html#abi-sys-bus-siox-devices-siox-x-y-status-errors)
* [/sys/bus/siox/devices/siox-X-Y/type](abi-testing.html#abi-sys-bus-siox-devices-siox-x-y-type)
* [/sys/bus/siox/devices/siox-X-Y/watchdog](abi-testing.html#abi-sys-bus-siox-devices-siox-x-y-watchdog)
* [/sys/bus/siox/devices/siox-X-Y/watchdog\_errors](abi-testing.html#abi-sys-bus-siox-devices-siox-x-y-watchdog-errors)
* [/sys/bus/siox/devices/siox-X-Y/outbytes](abi-testing.html#abi-sys-bus-siox-devices-siox-x-y-outbytes)

## ABI file testing/sysfs-bus-soundwire-master

Has the following ABI:

* [/sys/bus/soundwire/devices/sdw-master-<N>/revision](abi-testing.html#abi-sys-bus-soundwire-devices-sdw-master-n-revision)

## ABI file testing/sysfs-bus-soundwire-slave

Has the following ABI:

* [/sys/bus/soundwire/devices/sdw:.../status](abi-testing.html#abi-sys-bus-soundwire-devices-sdw-status)
* [/sys/bus/soundwire/devices/sdw:.../dev-properties/mipi\_revision](abi-testing.html#abi-sys-bus-soundwire-devices-sdw-dev-properties-mipi-revision)
* [/sys/bus/soundwire/devices/sdw:.../dp0/max\_word](abi-testing.html#abi-sys-bus-soundwire-devices-sdw-dp0-max-word)
* [/sys/bus/soundwire/devices/sdw:.../dp<N>\_src/max\_word](abi-testing.html#abi-sys-bus-soundwire-devices-sdw-dp-n-src-max-word)

## ABI file testing/sysfs-bus-spi-devices-spi-nor

Has the following ABI:

* [/sys/bus/spi/devices/.../spi-nor/jedec\_id](abi-testing.html#abi-sys-bus-spi-devices-spi-nor-jedec-id)
* [/sys/bus/spi/devices/.../spi-nor/manufacturer](abi-testing.html#abi-sys-bus-spi-devices-spi-nor-manufacturer)
* [/sys/bus/spi/devices/.../spi-nor/partname](abi-testing.html#abi-sys-bus-spi-devices-spi-nor-partname)
* [/sys/bus/spi/devices/.../spi-nor/sfdp](abi-testing.html#abi-sys-bus-spi-devices-spi-nor-sfdp)

## ABI file testing/sysfs-bus-surface\_aggregator-tabletsw

Has the following ABI:

* [/sys/bus/surface\_aggregator/devices/01:0e:01:00:01/state](abi-testing.html#abi-sys-bus-surface-aggregator-devices-01-0e-01-00-01-state)
* [/sys/bus/surface\_aggregator/devices/01:26:01:00:01/state](abi-testing.html#abi-sys-bus-surface-aggregator-devices-01-26-01-00-01-state)

## ABI file testing/sysfs-bus-thunderbolt

Has the following ABI:

* [/sys/bus/thunderbolt/devices/.../domainX/boot\_acl](abi-testing.html#abi-sys-bus-thunderbolt-devices-domainx-boot-acl)
* [/sys/bus/thunderbolt/devices/.../domainX/deauthorization](abi-testing.html#abi-sys-bus-thunderbolt-devices-domainx-deauthorization)
* [/sys/bus/thunderbolt/devices/.../domainX/iommu\_dma\_protection](abi-testing.html#abi-sys-bus-thunderbolt-devices-domainx-iommu-dma-protection)
* [/sys/bus/thunderbolt/devices/.../domainX/security](abi-testing.html#abi-sys-bus-thunderbolt-devices-domainx-security)
* [/sys/bus/thunderbolt/devices/.../authorized](abi-testing.html#abi-sys-bus-thunderbolt-devices-authorized)
* [/sys/bus/thunderbolt/devices/.../boot](abi-testing.html#abi-sys-bus-thunderbolt-devices-boot)
* [/sys/bus/thunderbolt/devices/.../generation](abi-testing.html#abi-sys-bus-thunderbolt-devices-generation)
* [/sys/bus/thunderbolt/devices/.../key](abi-testing.html#abi-sys-bus-thunderbolt-devices-key)
* [/sys/bus/thunderbolt/devices/.../device](abi-testing.html#abi-sys-bus-thunderbolt-devices-device)
* [/sys/bus/thunderbolt/devices/.../device\_name](abi-testing.html#abi-sys-bus-thunderbolt-devices-device-name)
* [/sys/bus/thunderbolt/devices/.../maxhopid](abi-testing.html#abi-sys-bus-thunderbolt-devices-maxhopid)
* [/sys/bus/thunderbolt/devices/.../rx\_speed](abi-testing.html#abi-sys-bus-thunderbolt-devices-rx-speed)
* [/sys/bus/thunderbolt/devices/.../rx\_lanes](abi-testing.html#abi-sys-bus-thunderbolt-devices-rx-lanes)
* [/sys/bus/thunderbolt/devices/.../tx\_speed](abi-testing.html#abi-sys-bus-thunderbolt-devices-tx-speed)
* [/sys/bus/thunderbolt/devices/.../tx\_lanes](abi-testing.html#abi-sys-bus-thunderbolt-devices-tx-lanes)
* [/sys/bus/thunderbolt/devices/.../vendor](abi-testing.html#abi-sys-bus-thunderbolt-devices-vendor)
* [/sys/bus/thunderbolt/devices/.../vendor\_name](abi-testing.html#abi-sys-bus-thunderbolt-devices-vendor-name)
* [/sys/bus/thunderbolt/devices/.../unique\_id](abi-testing.html#abi-sys-bus-thunderbolt-devices-unique-id)
* [/sys/bus/thunderbolt/devices/.../nvm\_version](abi-testing.html#abi-sys-bus-thunderbolt-devices-nvm-version)
* [/sys/bus/thunderbolt/devices/.../nvm\_authenticate](abi-testing.html#abi-sys-bus-thunderbolt-devices-nvm-authenticate)
* [/sys/bus/thunderbolt/devices/.../nvm\_authenticate\_on\_disconnect](abi-testing.html#abi-sys-bus-thunderbolt-devices-nvm-authenticate-on-disconnect)
* [/sys/bus/thunderbolt/devices/<xdomain>.<service>/key](abi-testing.html#abi-sys-bus-thunderbolt-devices-xdomain-service-key)
* [/sys/bus/thunderbolt/devices/<xdomain>.<service>/modalias](abi-testing.html#abi-sys-bus-thunderbolt-devices-xdomain-service-modalias)
* [/sys/bus/thunderbolt/devices/<xdomain>.<service>/prtcid](abi-testing.html#abi-sys-bus-thunderbolt-devices-xdomain-service-prtcid)
* [/sys/bus/thunderbolt/devices/<xdomain>.<service>/prtcvers](abi-testing.html#abi-sys-bus-thunderbolt-devices-xdomain-service-prtcvers)
* [/sys/bus/thunderbolt/devices/<xdomain>.<service>/prtcrevs](abi-testing.html#abi-sys-bus-thunderbolt-devices-xdomain-service-prtcrevs)
* [/sys/bus/thunderbolt/devices/<xdomain>.<service>/prtcstns](abi-testing.html#abi-sys-bus-thunderbolt-devices-xdomain-service-prtcstns)
* [/sys/bus/thunderbolt/devices/usb4\_portX/connector](abi-testing.html#abi-sys-bus-thunderbolt-devices-usb4-portx-connector)
* [/sys/bus/thunderbolt/devices/usb4\_portX/link](abi-testing.html#abi-sys-bus-thunderbolt-devices-usb4-portx-link)
* [/sys/bus/thunderbolt/devices/usb4\_portX/offline](abi-testing.html#abi-sys-bus-thunderbolt-devices-usb4-portx-offline)
* [/sys/bus/thunderbolt/devices/usb4\_portX/rescan](abi-testing.html#abi-sys-bus-thunderbolt-devices-usb4-portx-rescan)
* [/sys/bus/thunderbolt/devices/<device>:<port>.<index>/device](abi-testing.html#abi-sys-bus-thunderbolt-devices-device-port-index-device)
* [/sys/bus/thunderbolt/devices/<device>:<port>.<index>/nvm\_authenticate](abi-testing.html#abi-sys-bus-thunderbolt-devices-device-port-index-nvm-authenticate)
* [/sys/bus/thunderbolt/devices/<device>:<port>.<index>/nvm\_version](abi-testing.html#abi-sys-bus-thunderbolt-devices-device-port-index-nvm-version)
* [/sys/bus/thunderbolt/devices/<device>:<port>.<index>/vendor](abi-testing.html#abi-sys-bus-thunderbolt-devices-device-port-index-vendor)

## ABI file testing/sysfs-bus-typec

Has the following ABI:

* [/sys/bus/typec/devices/.../active](abi-testing.html#abi-sys-bus-typec-devices-active)
* [/sys/bus/typec/devices/.../description](abi-testing.html#abi-sys-bus-typec-devices-description)
* [/sys/bus/typec/devices/.../mode](abi-testing.html#abi-sys-bus-typec-devices-mode)
* [/sys/bus/typec/devices/.../svid](abi-testing.html#abi-sys-bus-typec-devices-svid)
* [/sys/bus/typec/devices/.../vdo](abi-testing.html#abi-sys-bus-typec-devices-vdo)

## ABI file testing/sysfs-bus-usb

Has the following ABI:

* [/sys/bus/usb/devices/<INTERFACE>/authorized](abi-testing.html#abi-sys-bus-usb-devices-interface-authorized)
* [/sys/bus/usb/devices/usbX/interface\_authorized\_default](abi-testing.html#abi-sys-bus-usb-devices-usbx-interface-authorized-default)
* [/sys/bus/usb/device/.../authorized](abi-testing.html#abi-sys-bus-usb-device-authorized)
* [/sys/bus/usb/drivers/.../new\_id](abi-testing.html#abi-sys-bus-usb-drivers-new-id)
* [/sys/bus/usb-serial/drivers/.../new\_id](abi-testing.html#abi-sys-bus-usb-serial-drivers-new-id)
* [/sys/bus/usb/drivers/.../remove\_id](abi-testing.html#abi-sys-bus-usb-drivers-remove-id)
* [/sys/bus/usb/devices/.../power/usb2\_hardware\_lpm](abi-testing.html#abi-sys-bus-usb-devices-power-usb2-hardware-lpm)
* [/sys/bus/usb/devices/.../power/usb3\_hardware\_lpm\_u1](abi-testing.html#abi-sys-bus-usb-devices-power-usb3-hardware-lpm-u1)
* [/sys/bus/usb/devices/.../ltm\_capable](abi-testing.html#abi-sys-bus-usb-devices-ltm-capable)
* [/sys/bus/usb/devices/<INTERFACE>/wireless\_status](abi-testing.html#abi-sys-bus-usb-devices-interface-wireless-status)
* [/sys/bus/usb/devices/.../<hub\_interface>/port<X>](abi-testing.html#abi-sys-bus-usb-devices-hub-interface-port-x)
* [/sys/bus/usb/devices/.../<hub\_interface>/port<X>/connect\_type](abi-testing.html#abi-sys-bus-usb-devices-hub-interface-port-x-connect-type)
* [/sys/bus/usb/devices/.../<hub\_interface>/port<X>/location](abi-testing.html#abi-sys-bus-usb-devices-hub-interface-port-x-location)
* [/sys/bus/usb/devices/.../<hub\_interface>/port<X>/quirks](abi-testing.html#abi-sys-bus-usb-devices-hub-interface-port-x-quirks)
* [/sys/bus/usb/devices/.../<hub\_interface>/port<X>/over\_current\_count](abi-testing.html#abi-sys-bus-usb-devices-hub-interface-port-x-over-current-count)
* [/sys/bus/usb/devices/.../<hub\_interface>/port<X>/usb3\_lpm\_permit](abi-testing.html#abi-sys-bus-usb-devices-hub-interface-port-x-usb3-lpm-permit)
* [/sys/bus/usb/devices/.../<hub\_interface>/port<X>/connector](abi-testing.html#abi-sys-bus-usb-devices-hub-interface-port-x-connector)
* [/sys/bus/usb/devices/.../<hub\_interface>/port<X>/disable](abi-testing.html#abi-sys-bus-usb-devices-hub-interface-port-x-disable)
* [/sys/bus/usb/devices/.../<hub\_interface>/port<X>/early\_stop](abi-testing.html#abi-sys-bus-usb-devices-hub-interface-port-x-early-stop)
* [/sys/bus/usb/devices/.../<hub\_interface>/port<X>/state](abi-testing.html#abi-sys-bus-usb-devices-hub-interface-port-x-state)
* [/sys/bus/usb/devices/.../power/usb2\_lpm\_l1\_timeout](abi-testing.html#abi-sys-bus-usb-devices-power-usb2-lpm-l1-timeout)
* [/sys/bus/usb/devices/.../power/usb2\_lpm\_besl](abi-testing.html#abi-sys-bus-usb-devices-power-usb2-lpm-besl)
* [/sys/bus/usb/devices/.../rx\_lanes](abi-testing.html#abi-sys-bus-usb-devices-rx-lanes)
* [/sys/bus/usb/devices/.../tx\_lanes](abi-testing.html#abi-sys-bus-usb-devices-tx-lanes)
* [/sys/bus/usb/devices/.../typec](abi-testing.html#abi-sys-bus-usb-devices-typec)
* [/sys/bus/usb/devices/usbX/bAlternateSetting](abi-testing.html#abi-sys-bus-usb-devices-usbx-balternatesetting)
* [/sys/bus/usb/devices/usbX/bcdDevice](abi-testing.html#abi-sys-bus-usb-devices-usbx-bcddevice)
* [/sys/bus/usb/devices/usbX/bConfigurationValue](abi-testing.html#abi-sys-bus-usb-devices-usbx-bconfigurationvalue)
* [/sys/bus/usb/devices/usbX/bDeviceClass](abi-testing.html#abi-sys-bus-usb-devices-usbx-bdeviceclass)
* [/sys/bus/usb/devices/usbX/bDeviceProtocol](abi-testing.html#abi-sys-bus-usb-devices-usbx-bdeviceprotocol)
* [/sys/bus/usb/devices/usbX/bDeviceSubClass](abi-testing.html#abi-sys-bus-usb-devices-usbx-bdevicesubclass)
* [/sys/bus/usb/devices/usbX/bInterfaceClass](abi-testing.html#abi-sys-bus-usb-devices-usbx-binterfaceclass)
* [/sys/bus/usb/devices/usbX/bInterfaceNumber](abi-testing.html#abi-sys-bus-usb-devices-usbx-binterfacenumber)
* [/sys/bus/usb/devices/usbX/bInterfaceProtocol](abi-testing.html#abi-sys-bus-usb-devices-usbx-binterfaceprotocol)
* [/sys/bus/usb/devices/usbX/bInterfaceSubClass](abi-testing.html#abi-sys-bus-usb-devices-usbx-binterfacesubclass)
* [/sys/bus/usb/devices/usbX/bmAttributes](abi-testing.html#abi-sys-bus-usb-devices-usbx-bmattributes)
* [/sys/bus/usb/devices/usbX/bMaxPacketSize0](abi-testing.html#abi-sys-bus-usb-devices-usbx-bmaxpacketsize0)
* [/sys/bus/usb/devices/usbX/bMaxPower](abi-testing.html#abi-sys-bus-usb-devices-usbx-bmaxpower)
* [/sys/bus/usb/devices/usbX/bNumConfigurations](abi-testing.html#abi-sys-bus-usb-devices-usbx-bnumconfigurations)
* [/sys/bus/usb/devices/usbX/bNumEndpoints](abi-testing.html#abi-sys-bus-usb-devices-usbx-bnumendpoints)
* [/sys/bus/usb/devices/usbX/bNumInterfaces](abi-testing.html#abi-sys-bus-usb-devices-usbx-bnuminterfaces)
* [/sys/bus/usb/devices/usbX/busnum](abi-testing.html#abi-sys-bus-usb-devices-usbx-busnum)
* [/sys/bus/usb/devices/usbX/configuration](abi-testing.html#abi-sys-bus-usb-devices-usbx-configuration)
* [/sys/bus/usb/devices/usbX/descriptors](abi-testing.html#abi-sys-bus-usb-devices-usbx-descriptors)
* [/sys/bus/usb/devices/usbX/bos\_descriptors](abi-testing.html#abi-sys-bus-usb-devices-usbx-bos-descriptors)
* [/sys/bus/usb/devices/usbX/idProduct](abi-testing.html#abi-sys-bus-usb-devices-usbx-idproduct)
* [/sys/bus/usb/devices/usbX/idVendor](abi-testing.html#abi-sys-bus-usb-devices-usbx-idvendor)
* [/sys/bus/usb/devices/usbX/devspec](abi-testing.html#abi-sys-bus-usb-devices-usbx-devspec)
* [/sys/bus/usb/devices/usbX/avoid\_reset\_quirk](abi-testing.html#abi-sys-bus-usb-devices-usbx-avoid-reset-quirk)
* [/sys/bus/usb/devices/usbX/devnum](abi-testing.html#abi-sys-bus-usb-devices-usbx-devnum)
* [/sys/bus/usb/devices/usbX/devpath](abi-testing.html#abi-sys-bus-usb-devices-usbx-devpath)
* [/sys/bus/usb/devices/usbX/manufacturer](abi-testing.html#abi-sys-bus-usb-devices-usbx-manufacturer)
* [/sys/bus/usb/devices/usbX/maxchild](abi-testing.html#abi-sys-bus-usb-devices-usbx-maxchild)
* [/sys/bus/usb/devices/usbX/persist](abi-testing.html#abi-sys-bus-usb-devices-usbx-persist)
* [/sys/bus/usb/devices/usbX/product](abi-testing.html#abi-sys-bus-usb-devices-usbx-product)
* [/sys/bus/usb/devices/usbX/speed](abi-testing.html#abi-sys-bus-usb-devices-usbx-speed)
* [/sys/bus/usb/devices/usbX/supports\_autosuspend](abi-testing.html#abi-sys-bus-usb-devices-usbx-supports-autosuspend)
* [/sys/bus/usb/devices/usbX/urbnum](abi-testing.html#abi-sys-bus-usb-devices-usbx-urbnum)
* [/sys/bus/usb/devices/usbX/version](abi-testing.html#abi-sys-bus-usb-devices-usbx-version)
* [/sys/bus/usb/devices/usbX/power/autosuspend](abi-testing.html#abi-sys-bus-usb-devices-usbx-power-autosuspend)
* [/sys/bus/usb/devices/usbX/power/active\_duration](abi-testing.html#abi-sys-bus-usb-devices-usbx-power-active-duration)
* [/sys/bus/usb/devices/usbX/power/connected\_duration](abi-testing.html#abi-sys-bus-usb-devices-usbx-power-connected-duration)
* [/sys/bus/usb/devices/usbX/power/level](abi-testing.html#abi-sys-bus-usb-devices-usbx-power-level)
* [/sys/bus/usb/devices/usbX/ep\_<N>/bEndpointAddress](abi-testing.html#abi-sys-bus-usb-devices-usbx-ep-n-bendpointaddress)
* [/sys/bus/usb/devices/usbX/ep\_<N>/bInterval](abi-testing.html#abi-sys-bus-usb-devices-usbx-ep-n-binterval)
* [/sys/bus/usb/devices/usbX/ep\_<N>/bLength](abi-testing.html#abi-sys-bus-usb-devices-usbx-ep-n-blength)
* [/sys/bus/usb/devices/usbX/ep\_<N>/bmAttributes](abi-testing.html#abi-sys-bus-usb-devices-usbx-ep-n-bmattributes)
* [/sys/bus/usb/devices/usbX/ep\_<N>/direction](abi-testing.html#abi-sys-bus-usb-devices-usbx-ep-n-direction)
* [/sys/bus/usb/devices/usbX/ep\_<N>/interval](abi-testing.html#abi-sys-bus-usb-devices-usbx-ep-n-interval)
* [/sys/bus/usb/devices/usbX/ep\_<N>/type](abi-testing.html#abi-sys-bus-usb-devices-usbx-ep-n-type)
* [/sys/bus/usb/devices/usbX/ep\_<N>/wMaxPacketSize](abi-testing.html#abi-sys-bus-usb-devices-usbx-ep-n-wmaxpacketsize)

## ABI file testing/sysfs-bus-usb-devices-usbsevseg

Has the following ABI:

* [/sys/bus/usb/.../powered](abi-testing.html#abi-sys-bus-usb-powered)
* [/sys/bus/usb/.../mode\_msb](abi-testing.html#abi-sys-bus-usb-mode-msb)
* [/sys/bus/usb/.../mode\_lsb](abi-testing.html#abi-sys-bus-usb-mode-msb)
* [/sys/bus/usb/.../textmode](abi-testing.html#abi-sys-bus-usb-textmode)
* [/sys/bus/usb/.../text](abi-testing.html#abi-sys-bus-usb-text)
* [/sys/bus/usb/.../decimals](abi-testing.html#abi-sys-bus-usb-decimals)

## ABI file testing/sysfs-bus-usb-lvstest

Link Layer Validation Device is a standard device for testing of Super
Speed Link Layer tests. These nodes are available in sysfs only when lvs
driver is bound with root hub device.

Has the following ABI:

* [/sys/bus/usb/devices/.../get\_dev\_desc](abi-testing.html#abi-sys-bus-usb-devices-get-dev-desc)
* [/sys/bus/usb/devices/.../u1\_timeout](abi-testing.html#abi-sys-bus-usb-devices-u1-timeout)
* [/sys/bus/usb/devices/.../u2\_timeout](abi-testing.html#abi-sys-bus-usb-devices-u2-timeout)
* [/sys/bus/usb/devices/.../hot\_reset](abi-testing.html#abi-sys-bus-usb-devices-hot-reset)
* [/sys/bus/usb/devices/.../u3\_entry](abi-testing.html#abi-sys-bus-usb-devices-u3-entry)
* [/sys/bus/usb/devices/.../u3\_exit](abi-testing.html#abi-sys-bus-usb-devices-u3-exit)
* [/sys/bus/usb/devices/.../enable\_compliance](abi-testing.html#abi-sys-bus-usb-devices-enable-compliance)
* [/sys/bus/usb/devices/.../warm\_reset](abi-testing.html#abi-sys-bus-usb-devices-warm-reset)

## ABI file testing/sysfs-bus-vdpa

Has the following ABI:

* [/sys/bus/vdpa/drivers\_autoprobe](abi-testing.html#abi-sys-bus-vdpa-drivers-autoprobe)
* [/sys/bus/vdpa/driver\_probe](abi-testing.html#abi-sys-bus-vdpa-driver-probe)
* [/sys/bus/vdpa/drivers/.../bind](abi-testing.html#abi-sys-bus-vdpa-drivers-bind)
* [/sys/bus/vdpa/drivers/.../unbind](abi-testing.html#abi-sys-bus-vdpa-drivers-unbind)
* [/sys/bus/vdpa/devices/.../driver\_override](abi-testing.html#abi-sys-bus-vdpa-devices-driver-override)

## ABI file testing/sysfs-bus-vfio-mdev

Has the following ABI:

* [/sys/.../<device>/mdev\_supported\_types/](abi-testing.html#abi-sys-device-mdev-supported-types)
* [/sys/.../<device>/mdev\_supported\_types/<type-id>/](abi-testing.html#abi-sys-device-mdev-supported-types-type-id)
* [/sys/.../mdev\_supported\_types/<type-id>/create](abi-testing.html#abi-sys-mdev-supported-types-type-id-create)
* [/sys/.../mdev\_supported\_types/<type-id>/devices/](abi-testing.html#abi-sys-mdev-supported-types-type-id-devices)
* [/sys/.../mdev\_supported\_types/<type-id>/available\_instances](abi-testing.html#abi-sys-mdev-supported-types-type-id-available-instances)
* [/sys/.../mdev\_supported\_types/<type-id>/device\_api](abi-testing.html#abi-sys-mdev-supported-types-type-id-device-api)
* [/sys/.../mdev\_supported\_types/<type-id>/name](abi-testing.html#abi-sys-mdev-supported-types-type-id-name)
* [/sys/.../mdev\_supported\_types/<type-id>/description](abi-testing.html#abi-sys-mdev-supported-types-type-id-description)
* [/sys/.../<device>/<UUID>/](abi-testing.html#abi-sys-device-uuid)
* [/sys/.../<device>/<UUID>/mdev\_type](abi-testing.html#abi-sys-device-uuid-mdev-type)
* [/sys/.../<device>/<UUID>/remove](abi-testing.html#abi-sys-device-uuid-remove)

## ABI file testing/sysfs-bus-vmbus

Has the following ABI:

* [/sys/bus/vmbus/devices/.../driver\_override](abi-testing.html#abi-sys-bus-vmbus-devices-driver-override)

## ABI file testing/sysfs-bus-wmi

Has the following ABI:

* [/sys/bus/wmi/devices/.../driver\_override](abi-testing.html#abi-sys-bus-wmi-devices-driver-override)
* [/sys/bus/wmi/devices/.../modalias](abi-testing.html#abi-sys-bus-wmi-devices-modalias)
* [/sys/bus/wmi/devices/.../guid](abi-testing.html#abi-sys-bus-wmi-devices-guid)
* [/sys/bus/wmi/devices/.../object\_id](abi-testing.html#abi-sys-bus-wmi-devices-object-id)
* [/sys/bus/wmi/devices/.../notify\_id](abi-testing.html#abi-sys-bus-wmi-devices-notify-id)
* [/sys/bus/wmi/devices/.../instance\_count](abi-testing.html#abi-sys-bus-wmi-devices-instance-count)
* [/sys/bus/wmi/devices/.../expensive](abi-testing.html#abi-sys-bus-wmi-devices-expensive)
* [/sys/bus/wmi/devices/.../setable](abi-testing.html#abi-sys-bus-wmi-devices-setable)

## ABI file testing/sysfs-c2port

Has the following ABI:

* [/sys/class/c2port/](abi-testing.html#abi-sys-class-c2port)
* [/sys/class/c2port/c2portX](abi-testing.html#abi-sys-class-c2port-c2portx)
* [/sys/class/c2port/c2portX/access](abi-testing.html#abi-sys-class-c2port-c2portx-access)
* [/sys/class/c2port/c2portX/dev\_id](abi-testing.html#abi-sys-class-c2port-c2portx-dev-id)
* [/sys/class/c2port/c2portX/flash\_access](abi-testing.html#abi-sys-class-c2port-c2portx-flash-access)
* [/sys/class/c2port/c2portX/flash\_block\_size](abi-testing.html#abi-sys-class-c2port-c2portx-flash-block-size)
* [/sys/class/c2port/c2portX/flash\_blocks\_num](abi-testing.html#abi-sys-class-c2port-c2portx-flash-blocks-num)
* [/sys/class/c2port/c2portX/flash\_data](abi-testing.html#abi-sys-class-c2port-c2portx-flash-data)
* [/sys/class/c2port/c2portX/flash\_erase](abi-testing.html#abi-sys-class-c2port-c2portx-flash-erase)
* [/sys/class/c2port/c2portX/reset](abi-testing.html#abi-sys-class-c2port-c2portx-reset)
* [/sys/class/c2port/c2portX/rev\_id](abi-testing.html#abi-sys-class-c2port-c2portx-rev-id)

## ABI file testing/sysfs-cfq-target-latency

Has the following ABI:

* [/sys/block/<device>/iosched/target\_latency](abi-testing.html#abi-sys-block-device-iosched-target-latency)

## ABI file testing/sysfs-class

Has the following ABI:

* [/sys/class/](abi-testing.html#abi-sys-class)

## ABI file testing/sysfs-class-backlight

Has the following ABI:

* [/sys/class/backlight/<backlight>/scale](abi-testing.html#abi-sys-class-backlight-backlight-scale)
* [/sys/class/backlight/<backlight>/ambient\_light\_level](abi-testing.html#abi-sys-class-backlight-backlight-ambient-light-level)
* [/sys/class/backlight/<backlight>/ambient\_light\_zone](abi-testing.html#abi-sys-class-backlight-backlight-ambient-light-zone)
* [/sys/class/backlight/<backlight>/<ambient light zone>\_max](abi-testing.html#abi-sys-class-backlight-backlight-ambient-light-zone-max)
* [/sys/class/backlight/<backlight>/<ambient light zone>\_dim](abi-testing.html#abi-sys-class-backlight-backlight-ambient-light-zone-dim)

## ABI file testing/sysfs-class-backlight-driver-lm3533

Has the following ABI:

* [/sys/class/backlight/<backlight>/als\_channel](abi-testing.html#abi-sys-class-backlight-backlight-als-channel)
* [/sys/class/backlight/<backlight>/als\_en](abi-testing.html#abi-sys-class-backlight-backlight-als-en)
* [/sys/class/backlight/<backlight>/id](abi-testing.html#abi-sys-class-backlight-backlight-id)
* [/sys/class/backlight/<backlight>/linear](abi-testing.html#abi-sys-class-backlight-backlight-linear)
* [/sys/class/backlight/<backlight>/pwm](abi-testing.html#abi-sys-class-backlight-backlight-pwm)

## ABI file testing/sysfs-class-backlight-lm3639

sysfs interface for Texas Instruments lm3639 backlight + flash led driver chip

Has the following ABI:

* [/sys/class/backlight/<backlight>/bled\_mode](abi-testing.html#abi-sys-class-backlight-backlight-bled-mode)

## ABI file testing/sysfs-class-bdi

Has the following ABI:

* [/sys/class/bdi/<bdi>/](abi-testing.html#abi-sys-class-bdi-bdi)
* [/sys/class/bdi/<bdi>/read\_ahead\_kb](abi-testing.html#abi-sys-class-bdi-bdi-read-ahead-kb)
* [/sys/class/bdi/<bdi>/min\_ratio](abi-testing.html#abi-sys-class-bdi-bdi-min-ratio)
* [/sys/class/bdi/<bdi>/min\_ratio\_fine](abi-testing.html#abi-sys-class-bdi-bdi-min-ratio-fine)
* [/sys/class/bdi/<bdi>/max\_ratio](abi-testing.html#abi-sys-class-bdi-bdi-max-ratio)
* [/sys/class/bdi/<bdi>/max\_ratio\_fine](abi-testing.html#abi-sys-class-bdi-bdi-max-ratio-fine)
* [/sys/class/bdi/<bdi>/min\_bytes](abi-testing.html#abi-sys-class-bdi-bdi-min-bytes)
* [/sys/class/bdi/<bdi>/max\_bytes](abi-testing.html#abi-sys-class-bdi-bdi-max-bytes)
* [/sys/class/bdi/<bdi>/strict\_limit](abi-testing.html#abi-sys-class-bdi-bdi-strict-limit)
* [/sys/class/bdi/<bdi>/stable\_pages\_required](abi-testing.html#abi-sys-class-bdi-bdi-stable-pages-required)

## ABI file testing/sysfs-class-bsr

Has the following ABI:

* [/sys/class/bsr/bsr\*/bsr\_size](abi-testing.html#abi-sys-class-bsr-bsr-bsr-size)
* [/sys/class/bsr/bsr\*/bsr\_length](abi-testing.html#abi-sys-class-bsr-bsr-bsr-length)
* [/sys/class/bsr/bsr\*/bsr\_stride](abi-testing.html#abi-sys-class-bsr-bsr-bsr-stride)

## ABI file testing/sysfs-class-chromeos

Has the following ABI:

* [/sys/class/chromeos/<ec-device-name>/flashinfo](abi-testing.html#abi-sys-class-chromeos-ec-device-name-flashinfo)
* [/sys/class/chromeos/<ec-device-name>/kb\_wake\_angle](abi-testing.html#abi-sys-class-chromeos-ec-device-name-kb-wake-angle)
* [/sys/class/chromeos/<ec-device-name>/reboot](abi-testing.html#abi-sys-class-chromeos-ec-device-name-reboot)
* [/sys/class/chromeos/<ec-device-name>/version](abi-testing.html#abi-sys-class-chromeos-ec-device-name-version)
* [/sys/class/chromeos/cros\_ec/usbpdmuxinfo](abi-testing.html#abi-sys-class-chromeos-cros-ec-usbpdmuxinfo)
* [/sys/class/chromeos/cros\_ec/ap\_mode\_entry](abi-testing.html#abi-sys-class-chromeos-cros-ec-ap-mode-entry)

## ABI file testing/sysfs-class-chromeos-driver-cros-ec-lightbar

Has the following ABI:

* [/sys/class/chromeos/<ec-device-name>/lightbar/brightness](abi-testing.html#abi-sys-class-chromeos-ec-device-name-lightbar-brightness)
* [/sys/class/chromeos/<ec-device-name>/lightbar/interval\_msec](abi-testing.html#abi-sys-class-chromeos-ec-device-name-lightbar-interval-msec)
* [/sys/class/chromeos/<ec-device-name>/lightbar/led\_rgb](abi-testing.html#abi-sys-class-chromeos-ec-device-name-lightbar-led-rgb)
* [/sys/class/chromeos/<ec-device-name>/lightbar/program](abi-testing.html#abi-sys-class-chromeos-ec-device-name-lightbar-program)
* [/sys/class/chromeos/<ec-device-name>/lightbar/sequence](abi-testing.html#abi-sys-class-chromeos-ec-device-name-lightbar-sequence)
* [/sys/class/chromeos/<ec-device-name>/lightbar/userspace\_control](abi-testing.html#abi-sys-class-chromeos-ec-device-name-lightbar-userspace-control)
* [/sys/class/chromeos/<ec-device-name>/lightbar/version](abi-testing.html#abi-sys-class-chromeos-ec-device-name-lightbar-version)

## ABI file testing/sysfs-class-chromeos-driver-cros-ec-vbc

Has the following ABI:

* [/sys/class/chromeos/<ec-device-name>/vbc/vboot\_context](abi-testing.html#abi-sys-class-chromeos-ec-device-name-vbc-vboot-context)

## ABI file testing/sysfs-class-devfreq

Has the following ABI:

* [/sys/class/devfreq/.../](abi-testing.html#abi-sys-class-devfreq)
* [/sys/class/devfreq/.../name](abi-testing.html#abi-sys-class-devfreq-name)
* [/sys/class/devfreq/.../governor](abi-testing.html#abi-sys-class-devfreq-governor)
* [/sys/class/devfreq/.../cur\_freq](abi-testing.html#abi-sys-class-devfreq-cur-freq)
* [/sys/class/devfreq/.../target\_freq](abi-testing.html#abi-sys-class-devfreq-target-freq)
* [/sys/class/devfreq/.../trans\_stat](abi-testing.html#abi-sys-class-devfreq-trans-stat)
* [/sys/class/devfreq/.../available\_frequencies](abi-testing.html#abi-sys-class-devfreq-available-frequencies)
* [/sys/class/devfreq/.../available\_governors](abi-testing.html#abi-sys-class-devfreq-available-governors)
* [/sys/class/devfreq/.../min\_freq](abi-testing.html#abi-sys-class-devfreq-min-freq)
* [/sys/class/devfreq/.../max\_freq](abi-testing.html#abi-sys-class-devfreq-max-freq)
* [/sys/class/devfreq/.../polling\_interval](abi-testing.html#abi-sys-class-devfreq-polling-interval)
* [/sys/class/devfreq/.../userspace/set\_freq](abi-testing.html#abi-sys-class-devfreq-userspace-set-freq)
* [/sys/class/devfreq/.../timer](abi-testing.html#abi-sys-class-devfreq-timer)
* [/sys/class/devfreq/.../related\_cpus](abi-testing.html#abi-sys-class-devfreq-related-cpus)

## ABI file testing/sysfs-class-devfreq-event

Has the following ABI:

* [/sys/class/devfreq-event/event<x>/](abi-testing.html#abi-sys-class-devfreq-event-event-x)
* [/sys/class/devfreq-event/event<x>/name](abi-testing.html#abi-sys-class-devfreq-event-event-x-name)
* [/sys/class/devfreq-event/event<x>/enable\_count](abi-testing.html#abi-sys-class-devfreq-event-event-x-enable-count)

## ABI file testing/sysfs-class-devlink

Has the following ABI:

* [/sys/class/devlink/.../](abi-testing.html#abi-sys-class-devlink)
* [/sys/class/devlink/.../auto\_remove\_on](abi-testing.html#abi-sys-class-devlink-auto-remove-on)
* [/sys/class/devlink/.../consumer](abi-testing.html#abi-sys-class-devlink-consumer)
* [/sys/class/devlink/.../runtime\_pm](abi-testing.html#abi-sys-class-devlink-runtime-pm)
* [/sys/class/devlink/.../status](abi-testing.html#abi-sys-class-devlink-status)
* [/sys/class/devlink/.../supplier](abi-testing.html#abi-sys-class-devlink-supplier)
* [/sys/class/devlink/.../sync\_state\_only](abi-testing.html#abi-sys-class-devlink-sync-state-only)

## ABI file testing/sysfs-class-drm

Has the following ABI:

* [/sys/class/drm/.../boot\_display](abi-testing.html#abi-sys-class-drm-boot-display)

## ABI file testing/sysfs-class-extcon

Has the following ABI:

* [/sys/class/extcon/.../](abi-testing.html#abi-sys-class-extcon)
* [/sys/class/extcon/.../name](abi-testing.html#abi-sys-class-extcon-name)
* [/sys/class/extcon/.../state](abi-testing.html#abi-sys-class-extcon-state)
* [/sys/class/extcon/.../cable.X/name](abi-testing.html#abi-sys-class-extcon-cable-x-name)
* [/sys/class/extcon/.../cable.X/state](abi-testing.html#abi-sys-class-extcon-cable-x-state)
* [/sys/class/extcon/.../mutually\_exclusive/...](abi-testing.html#abi-sys-class-extcon-mutually-exclusive)

## ABI file testing/sysfs-class-fc

Has the following ABI:

* [/sys/class/fc/fc\_udev\_device/appid\_store](abi-testing.html#abi-sys-class-fc-fc-udev-device-appid-store)

## ABI file testing/sysfs-class-fc\_host

Has the following ABI:

* [/sys/class/fc\_host/hostX/statistics/fpin\_cn\_yyy](abi-testing.html#abi-sys-class-fc-host-hostx-statistics-fpin-cn-yyy)
* [/sys/class/fc\_host/hostX/statistics/fpin\_li\_yyy](abi-testing.html#abi-sys-class-fc-host-hostx-statistics-fpin-li-yyy)
* [/sys/class/fc\_host/hostX/statistics/fpin\_dn\_yyy](abi-testing.html#abi-sys-class-fc-host-hostx-statistics-fpin-dn-yyy)

## ABI file testing/sysfs-class-fc\_remote\_ports

Has the following ABI:

* [/sys/class/fc\_remote\_ports/rport-X:Y-Z/statistics/fpin\_cn\_yyy](abi-testing.html#abi-sys-class-fc-remote-ports-rport-x-y-z-statistics-fpin-cn-yyy)
* [/sys/class/fc\_remote\_ports/rport-X:Y-Z/statistics/fpin\_li\_yyy](abi-testing.html#abi-sys-class-fc-remote-ports-rport-x-y-z-statistics-fpin-li-yyy)
* [/sys/class/fc\_remote\_ports/rport-X:Y-Z/statistics/fpin\_dn\_yyy](abi-testing.html#abi-sys-class-fc-remote-ports-rport-x-y-z-statistics-fpin-dn-yyy)

## ABI file testing/sysfs-class-firmware

Has the following ABI:

* [/sys/class/firmware/.../data](abi-testing.html#abi-sys-class-firmware-data)
* [/sys/class/firmware/.../cancel](abi-testing.html#abi-sys-class-firmware-cancel)
* [/sys/class/firmware/.../error](abi-testing.html#abi-sys-class-firmware-error)
* [/sys/class/firmware/.../loading](abi-testing.html#abi-sys-class-firmware-loading)
* [/sys/class/firmware/.../remaining\_size](abi-testing.html#abi-sys-class-firmware-remaining-size)
* [/sys/class/firmware/.../status](abi-testing.html#abi-sys-class-firmware-status)
* [/sys/class/firmware/.../timeout](abi-testing.html#abi-sys-class-firmware-timeout)

## ABI file testing/sysfs-class-firmware-attributes

Has the following ABI:

* [/sys/class/firmware-attributes/\*/attributes/\*/](abi-testing.html#abi-sys-class-firmware-attributes-attributes)
* [/sys/class/firmware-attributes/\*/authentication/](abi-testing.html#abi-sys-class-firmware-attributes-authentication)
* [/sys/class/firmware-attributes/\*/attributes/pending\_reboot](abi-testing.html#abi-sys-class-firmware-attributes-attributes-pending-reboot)
* [/sys/class/firmware-attributes/\*/attributes/reset\_bios](abi-testing.html#abi-sys-class-firmware-attributes-attributes-reset-bios)
* [/sys/class/firmware-attributes/\*/attributes/save\_settings](abi-testing.html#abi-sys-class-firmware-attributes-attributes-save-settings)
* [/sys/class/firmware-attributes/\*/attributes/debug\_cmd](abi-testing.html#abi-sys-class-firmware-attributes-attributes-debug-cmd)
* [/sys/class/firmware-attributes/\*/authentication/SPM/kek](abi-testing.html#abi-sys-class-firmware-attributes-authentication-spm-kek)
* [/sys/class/firmware-attributes/\*/authentication/SPM/sk](abi-testing.html#abi-sys-class-firmware-attributes-authentication-spm-sk)
* [/sys/class/firmware-attributes/\*/authentication/SPM/status](abi-testing.html#abi-sys-class-firmware-attributes-authentication-spm-status)
* [/sys/class/firmware-attributes/\*/attributes/Sure\_Start/audit\_log\_entries](abi-testing.html#abi-sys-class-firmware-attributes-attributes-sure-start-audit-log-entries)
* [/sys/class/firmware-attributes/\*/attributes/Sure\_Start/audit\_log\_entry\_count](abi-testing.html#abi-sys-class-firmware-attributes-attributes-sure-start-audit-log-entry-count)

## ABI file testing/sysfs-class-fpga-bridge

Has the following ABI:

* [/sys/class/fpga\_bridge/<bridge>/name](abi-testing.html#abi-sys-class-fpga-bridge-bridge-name)
* [/sys/class/fpga\_bridge/<bridge>/state](abi-testing.html#abi-sys-class-fpga-bridge-bridge-state)

## ABI file testing/sysfs-class-fpga-manager

Has the following ABI:

* [/sys/class/fpga\_manager/<fpga>/name](abi-testing.html#abi-sys-class-fpga-manager-fpga-name)
* [/sys/class/fpga\_manager/<fpga>/state](abi-testing.html#abi-sys-class-fpga-manager-fpga-state)
* [/sys/class/fpga\_manager/<fpga>/status](abi-testing.html#abi-sys-class-fpga-manager-fpga-status)

## ABI file testing/sysfs-class-fpga-region

Has the following ABI:

* [/sys/class/fpga\_region/<region>/compat\_id](abi-testing.html#abi-sys-class-fpga-region-region-compat-id)

## ABI file testing/sysfs-class-gnss

Has the following ABI:

* [/sys/class/gnss/gnss<N>/type](abi-testing.html#abi-sys-class-gnss-gnss-n-type)

## ABI file testing/sysfs-class-hwmon

Has the following ABI:

* [/sys/class/hwmon/hwmonX/name](abi-testing.html#abi-sys-class-hwmon-hwmonx-name)
* [/sys/class/hwmon/hwmonX/label](abi-testing.html#abi-sys-class-hwmon-hwmonx-label)
* [/sys/class/hwmon/hwmonX/update\_interval](abi-testing.html#abi-sys-class-hwmon-hwmonx-update-interval)
* [/sys/class/hwmon/hwmonX/inY\_min](abi-testing.html#abi-sys-class-hwmon-hwmonx-iny-min)
* [/sys/class/hwmon/hwmonX/inY\_lcrit](abi-testing.html#abi-sys-class-hwmon-hwmonx-iny-lcrit)
* [/sys/class/hwmon/hwmonX/inY\_max](abi-testing.html#abi-sys-class-hwmon-hwmonx-iny-max)
* [/sys/class/hwmon/hwmonX/inY\_crit](abi-testing.html#abi-sys-class-hwmon-hwmonx-iny-crit)
* [/sys/class/hwmon/hwmonX/inY\_input](abi-testing.html#abi-sys-class-hwmon-hwmonx-iny-input)
* [/sys/class/hwmon/hwmonX/inY\_average](abi-testing.html#abi-sys-class-hwmon-hwmonx-iny-average)
* [/sys/class/hwmon/hwmonX/inY\_lowest](abi-testing.html#abi-sys-class-hwmon-hwmonx-iny-lowest)
* [/sys/class/hwmon/hwmonX/inY\_highest](abi-testing.html#abi-sys-class-hwmon-hwmonx-iny-highest)
* [/sys/class/hwmon/hwmonX/inY\_reset\_history](abi-testing.html#abi-sys-class-hwmon-hwmonx-iny-reset-history)
* [/sys/class/hwmon/hwmonX/in\_reset\_history](abi-testing.html#abi-sys-class-hwmon-hwmonx-in-reset-history)
* [/sys/class/hwmon/hwmonX/inY\_label](abi-testing.html#abi-sys-class-hwmon-hwmonx-iny-label)
* [/sys/class/hwmon/hwmonX/inY\_enable](abi-testing.html#abi-sys-class-hwmon-hwmonx-iny-enable)
* [/sys/class/hwmon/hwmonX/inY\_fault](abi-testing.html#abi-sys-class-hwmon-hwmonx-iny-fault)
* [/sys/class/hwmon/hwmonX/cpuY\_vid](abi-testing.html#abi-sys-class-hwmon-hwmonx-cpuy-vid)
* [/sys/class/hwmon/hwmonX/vrm](abi-testing.html#abi-sys-class-hwmon-hwmonx-vrm)
* [/sys/class/hwmon/hwmonX/inY\_rated\_min](abi-testing.html#abi-sys-class-hwmon-hwmonx-iny-rated-min)
* [/sys/class/hwmon/hwmonX/inY\_rated\_max](abi-testing.html#abi-sys-class-hwmon-hwmonx-iny-rated-max)
* [/sys/class/hwmon/hwmonX/fanY\_min](abi-testing.html#abi-sys-class-hwmon-hwmonx-fany-min)
* [/sys/class/hwmon/hwmonX/fanY\_max](abi-testing.html#abi-sys-class-hwmon-hwmonx-fany-max)
* [/sys/class/hwmon/hwmonX/fanY\_input](abi-testing.html#abi-sys-class-hwmon-hwmonx-fany-input)
* [/sys/class/hwmon/hwmonX/fanY\_div](abi-testing.html#abi-sys-class-hwmon-hwmonx-fany-div)
* [/sys/class/hwmon/hwmonX/fanY\_pulses](abi-testing.html#abi-sys-class-hwmon-hwmonx-fany-pulses)
* [/sys/class/hwmon/hwmonX/fanY\_target](abi-testing.html#abi-sys-class-hwmon-hwmonx-fany-target)
* [/sys/class/hwmon/hwmonX/fanY\_label](abi-testing.html#abi-sys-class-hwmon-hwmonx-fany-label)
* [/sys/class/hwmon/hwmonX/fanY\_enable](abi-testing.html#abi-sys-class-hwmon-hwmonx-fany-enable)
* [/sys/class/hwmon/hwmonX/fanY\_fault](abi-testing.html#abi-sys-class-hwmon-hwmonx-fany-fault)
* [/sys/class/hwmon/hwmonX/pwmY](abi-testing.html#abi-sys-class-hwmon-hwmonx-pwmy)
* [/sys/class/hwmon/hwmonX/pwmY\_enable](abi-testing.html#abi-sys-class-hwmon-hwmonx-pwmy-enable)
* [/sys/class/hwmon/hwmonX/pwmY\_mode](abi-testing.html#abi-sys-class-hwmon-hwmonx-pwmy-mode)
* [/sys/class/hwmon/hwmonX/pwmY\_freq](abi-testing.html#abi-sys-class-hwmon-hwmonx-pwmy-freq)
* [/sys/class/hwmon/hwmonX/pwmY\_auto\_channels\_temp](abi-testing.html#abi-sys-class-hwmon-hwmonx-pwmy-auto-channels-temp)
* [/sys/class/hwmon/hwmonX/pwmY\_auto\_pointZ\_pwm](abi-testing.html#abi-sys-class-hwmon-hwmonx-pwmy-auto-pointz-pwm)
* [/sys/class/hwmon/hwmonX/pwmY\_auto\_pointZ\_temp](abi-testing.html#abi-sys-class-hwmon-hwmonx-pwmy-auto-pointz-pwm)
* [/sys/class/hwmon/hwmonX/pwmY\_auto\_pointZ\_temp\_hyst](abi-testing.html#abi-sys-class-hwmon-hwmonx-pwmy-auto-pointz-pwm)
* [/sys/class/hwmon/hwmonX/tempY\_auto\_pointZ\_pwm](abi-testing.html#abi-sys-class-hwmon-hwmonx-tempy-auto-pointz-pwm)
* [/sys/class/hwmon/hwmonX/tempY\_auto\_pointZ\_temp](abi-testing.html#abi-sys-class-hwmon-hwmonx-tempy-auto-pointz-pwm)
* [/sys/class/hwmon/hwmonX/tempY\_auto\_pointZ\_temp\_hyst](abi-testing.html#abi-sys-class-hwmon-hwmonx-tempy-auto-pointz-pwm)
* [/sys/class/hwmon/hwmonX/tempY\_type](abi-testing.html#abi-sys-class-hwmon-hwmonx-tempy-type)
* [/sys/class/hwmon/hwmonX/tempY\_max](abi-testing.html#abi-sys-class-hwmon-hwmonx-tempy-max)
* [/sys/class/hwmon/hwmonX/tempY\_max\_alarm](abi-testing.html#abi-sys-class-hwmon-hwmonx-tempy-max-alarm)
* [/sys/class/hwmon/hwmonX/tempY\_min](abi-testing.html#abi-sys-class-hwmon-hwmonx-tempy-min)
* [/sys/class/hwmon/hwmonX/tempY\_min\_alarm](abi-testing.html#abi-sys-class-hwmon-hwmonx-tempy-min-alarm)
* [/sys/class/hwmon/hwmonX/tempY\_max\_hyst](abi-testing.html#abi-sys-class-hwmon-hwmonx-tempy-max-hyst)
* [/sys/class/hwmon/hwmonX/tempY\_min\_hyst](abi-testing.html#abi-sys-class-hwmon-hwmonx-tempy-min-hyst)
* [/sys/class/hwmon/hwmonX/tempY\_input](abi-testing.html#abi-sys-class-hwmon-hwmonx-tempy-input)
* [/sys/class/hwmon/hwmonX/tempY\_crit](abi-testing.html#abi-sys-class-hwmon-hwmonx-tempy-crit)
* [/sys/class/hwmon/hwmonX/tempY\_crit\_alarm](abi-testing.html#abi-sys-class-hwmon-hwmonx-tempy-crit-alarm)
* [/sys/class/hwmon/hwmonX/tempY\_crit\_hyst](abi-testing.html#abi-sys-class-hwmon-hwmonx-tempy-crit-hyst)
* [/sys/class/hwmon/hwmonX/tempY\_emergency](abi-testing.html#abi-sys-class-hwmon-hwmonx-tempy-emergency)
* [/sys/class/hwmon/hwmonX/tempY\_emergency\_alarm](abi-testing.html#abi-sys-class-hwmon-hwmonx-tempy-emergency-alarm)
* [/sys/class/hwmon/hwmonX/tempY\_emergency\_hyst](abi-testing.html#abi-sys-class-hwmon-hwmonx-tempy-emergency-hyst)
* [/sys/class/hwmon/hwmonX/tempY\_lcrit](abi-testing.html#abi-sys-class-hwmon-hwmonx-tempy-lcrit)
* [/sys/class/hwmon/hwmonX/tempY\_lcrit\_hyst](abi-testing.html#abi-sys-class-hwmon-hwmonx-tempy-lcrit-hyst)
* [/sys/class/hwmon/hwmonX/tempY\_offset](abi-testing.html#abi-sys-class-hwmon-hwmonx-tempy-offset)
* [/sys/class/hwmon/hwmonX/tempY\_label](abi-testing.html#abi-sys-class-hwmon-hwmonx-tempy-label)
* [/sys/class/hwmon/hwmonX/tempY\_lowest](abi-testing.html#abi-sys-class-hwmon-hwmonx-tempy-lowest)
* [/sys/class/hwmon/hwmonX/tempY\_highest](abi-testing.html#abi-sys-class-hwmon-hwmonx-tempy-highest)
* [/sys/class/hwmon/hwmonX/tempY\_reset\_history](abi-testing.html#abi-sys-class-hwmon-hwmonx-tempy-reset-history)
* [/sys/class/hwmon/hwmonX/temp\_reset\_history](abi-testing.html#abi-sys-class-hwmon-hwmonx-temp-reset-history)
* [/sys/class/hwmon/hwmonX/tempY\_enable](abi-testing.html#abi-sys-class-hwmon-hwmonx-tempy-enable)
* [/sys/class/hwmon/hwmonX/tempY\_rated\_min](abi-testing.html#abi-sys-class-hwmon-hwmonx-tempy-rated-min)
* [/sys/class/hwmon/hwmonX/tempY\_rated\_max](abi-testing.html#abi-sys-class-hwmon-hwmonx-tempy-rated-max)
* [/sys/class/hwmon/hwmonX/currY\_max](abi-testing.html#abi-sys-class-hwmon-hwmonx-curry-max)
* [/sys/class/hwmon/hwmonX/currY\_min](abi-testing.html#abi-sys-class-hwmon-hwmonx-curry-min)
* [/sys/class/hwmon/hwmonX/currY\_lcrit](abi-testing.html#abi-sys-class-hwmon-hwmonx-curry-lcrit)
* [/sys/class/hwmon/hwmonX/currY\_crit](abi-testing.html#abi-sys-class-hwmon-hwmonx-curry-crit)
* [/sys/class/hwmon/hwmonX/currY\_input](abi-testing.html#abi-sys-class-hwmon-hwmonx-curry-input)
* [/sys/class/hwmon/hwmonX/currY\_average](abi-testing.html#abi-sys-class-hwmon-hwmonx-curry-average)
* [/sys/class/hwmon/hwmonX/currY\_lowest](abi-testing.html#abi-sys-class-hwmon-hwmonx-curry-lowest)
* [/sys/class/hwmon/hwmonX/currY\_highest](abi-testing.html#abi-sys-class-hwmon-hwmonx-curry-highest)
* [/sys/class/hwmon/hwmonX/currY\_reset\_history](abi-testing.html#abi-sys-class-hwmon-hwmonx-curry-reset-history)
* [/sys/class/hwmon/hwmonX/curr\_reset\_history](abi-testing.html#abi-sys-class-hwmon-hwmonx-curr-reset-history)
* [/sys/class/hwmon/hwmonX/currY\_enable](abi-testing.html#abi-sys-class-hwmon-hwmonx-curry-enable)
* [/sys/class/hwmon/hwmonX/currY\_rated\_min](abi-testing.html#abi-sys-class-hwmon-hwmonx-curry-rated-min)
* [/sys/class/hwmon/hwmonX/currY\_rated\_max](abi-testing.html#abi-sys-class-hwmon-hwmonx-curry-rated-max)
* [/sys/class/hwmon/hwmonX/powerY\_average](abi-testing.html#abi-sys-class-hwmon-hwmonx-powery-average)
* [/sys/class/hwmon/hwmonX/powerY\_average\_interval](abi-testing.html#abi-sys-class-hwmon-hwmonx-powery-average-interval)
* [/sys/class/hwmon/hwmonX/powerY\_average\_interval\_max](abi-testing.html#abi-sys-class-hwmon-hwmonx-powery-average-interval-max)
* [/sys/class/hwmon/hwmonX/powerY\_average\_interval\_min](abi-testing.html#abi-sys-class-hwmon-hwmonx-powery-average-interval-min)
* [/sys/class/hwmon/hwmonX/powerY\_average\_highest](abi-testing.html#abi-sys-class-hwmon-hwmonx-powery-average-highest)
* [/sys/class/hwmon/hwmonX/powerY\_average\_lowest](abi-testing.html#abi-sys-class-hwmon-hwmonx-powery-average-lowest)
* [/sys/class/hwmon/hwmonX/powerY\_average\_max](abi-testing.html#abi-sys-class-hwmon-hwmonx-powery-average-max)
* [/sys/class/hwmon/hwmonX/powerY\_average\_min](abi-testing.html#abi-sys-class-hwmon-hwmonx-powery-average-min)
* [/sys/class/hwmon/hwmonX/powerY\_input](abi-testing.html#abi-sys-class-hwmon-hwmonx-powery-input)
* [/sys/class/hwmon/hwmonX/powerY\_input\_highest](abi-testing.html#abi-sys-class-hwmon-hwmonx-powery-input-highest)
* [/sys/class/hwmon/hwmonX/powerY\_input\_lowest](abi-testing.html#abi-sys-class-hwmon-hwmonx-powery-input-lowest)
* [/sys/class/hwmon/hwmonX/powerY\_reset\_history](abi-testing.html#abi-sys-class-hwmon-hwmonx-powery-reset-history)
* [/sys/class/hwmon/hwmonX/powerY\_accuracy](abi-testing.html#abi-sys-class-hwmon-hwmonx-powery-accuracy)
* [/sys/class/hwmon/hwmonX/powerY\_cap](abi-testing.html#abi-sys-class-hwmon-hwmonx-powery-cap)
* [/sys/class/hwmon/hwmonX/powerY\_cap\_hyst](abi-testing.html#abi-sys-class-hwmon-hwmonx-powery-cap-hyst)
* [/sys/class/hwmon/hwmonX/powerY\_cap\_max](abi-testing.html#abi-sys-class-hwmon-hwmonx-powery-cap-max)
* [/sys/class/hwmon/hwmonX/powerY\_cap\_min](abi-testing.html#abi-sys-class-hwmon-hwmonx-powery-cap-min)
* [/sys/class/hwmon/hwmonX/powerY\_max](abi-testing.html#abi-sys-class-hwmon-hwmonx-powery-max)
* [/sys/class/hwmon/hwmonX/powerY\_crit](abi-testing.html#abi-sys-class-hwmon-hwmonx-powery-crit)
* [/sys/class/hwmon/hwmonX/powerY\_enable](abi-testing.html#abi-sys-class-hwmon-hwmonx-powery-enable)
* [/sys/class/hwmon/hwmonX/powerY\_rated\_min](abi-testing.html#abi-sys-class-hwmon-hwmonx-powery-rated-min)
* [/sys/class/hwmon/hwmonX/powerY\_rated\_max](abi-testing.html#abi-sys-class-hwmon-hwmonx-powery-rated-max)
* [/sys/class/hwmon/hwmonX/energyY\_input](abi-testing.html#abi-sys-class-hwmon-hwmonx-energyy-input)
* [/sys/class/hwmon/hwmonX/energyY\_enable](abi-testing.html#abi-sys-class-hwmon-hwmonx-energyy-enable)
* [/sys/class/hwmon/hwmonX/humidityY\_alarm](abi-testing.html#abi-sys-class-hwmon-hwmonx-humidityy-alarm)
* [/sys/class/hwmon/hwmonX/humidityY\_enable](abi-testing.html#abi-sys-class-hwmon-hwmonx-humidityy-enable)
* [/sys/class/hwmon/hwmonX/humidityY\_fault](abi-testing.html#abi-sys-class-hwmon-hwmonx-humidityy-fault)
* [/sys/class/hwmon/hwmonX/humidityY\_input](abi-testing.html#abi-sys-class-hwmon-hwmonx-humidityy-input)
* [/sys/class/hwmon/hwmonX/humidityY\_label](abi-testing.html#abi-sys-class-hwmon-hwmonx-humidityy-label)
* [/sys/class/hwmon/hwmonX/humidityY\_max](abi-testing.html#abi-sys-class-hwmon-hwmonx-humidityy-max)
* [/sys/class/hwmon/hwmonX/humidityY\_max\_alarm](abi-testing.html#abi-sys-class-hwmon-hwmonx-humidityy-max-alarm)
* [/sys/class/hwmon/hwmonX/humidityY\_max\_hyst](abi-testing.html#abi-sys-class-hwmon-hwmonx-humidityy-max-hyst)
* [/sys/class/hwmon/hwmonX/humidityY\_min](abi-testing.html#abi-sys-class-hwmon-hwmonx-humidityy-min)
* [/sys/class/hwmon/hwmonX/humidityY\_min\_alarm](abi-testing.html#abi-sys-class-hwmon-hwmonx-humidityy-min-alarm)
* [/sys/class/hwmon/hwmonX/humidityY\_min\_hyst](abi-testing.html#abi-sys-class-hwmon-hwmonx-humidityy-min-hyst)
* [/sys/class/hwmon/hwmonX/humidityY\_rated\_min](abi-testing.html#abi-sys-class-hwmon-hwmonx-humidityy-rated-min)
* [/sys/class/hwmon/hwmonX/humidityY\_rated\_max](abi-testing.html#abi-sys-class-hwmon-hwmonx-humidityy-rated-max)
* [/sys/class/hwmon/hwmonX/intrusionY\_alarm](abi-testing.html#abi-sys-class-hwmon-hwmonx-intrusiony-alarm)
* [/sys/class/hwmon/hwmonX/intrusionY\_beep](abi-testing.html#abi-sys-class-hwmon-hwmonx-intrusiony-beep)
* [/sys/class/hwmon/hwmonX/device/pec](abi-testing.html#abi-sys-class-hwmon-hwmonx-device-pec)

## ABI file testing/sysfs-class-intel\_pmt

Has the following ABI:

* [/sys/class/intel\_pmt/](abi-testing.html#abi-sys-class-intel-pmt)
* [/sys/class/intel\_pmt/telem<x>](abi-testing.html#abi-sys-class-intel-pmt-telem-x)
* [/sys/class/intel\_pmt/telem<x>/telem](abi-testing.html#abi-sys-class-intel-pmt-telem-x-telem)
* [/sys/class/intel\_pmt/telem<x>/guid](abi-testing.html#abi-sys-class-intel-pmt-telem-x-guid)
* [/sys/class/intel\_pmt/telem<x>/size](abi-testing.html#abi-sys-class-intel-pmt-telem-x-size)
* [/sys/class/intel\_pmt/telem<x>/offset](abi-testing.html#abi-sys-class-intel-pmt-telem-x-offset)
* [/sys/class/intel\_pmt/crashlog<x>](abi-testing.html#abi-sys-class-intel-pmt-crashlog-x)
* [/sys/class/intel\_pmt/crashlog<x>/crashlog](abi-testing.html#abi-sys-class-intel-pmt-crashlog-x-crashlog)
* [/sys/class/intel\_pmt/crashlog<x>/guid](abi-testing.html#abi-sys-class-intel-pmt-crashlog-x-guid)
* [/sys/class/intel\_pmt/crashlog<x>/size](abi-testing.html#abi-sys-class-intel-pmt-crashlog-x-size)
* [/sys/class/intel\_pmt/crashlog<x>/offset](abi-testing.html#abi-sys-class-intel-pmt-crashlog-x-offset)
* [/sys/class/intel\_pmt/crashlog<x>/enable](abi-testing.html#abi-sys-class-intel-pmt-crashlog-x-enable)
* [/sys/class/intel\_pmt/crashlog<x>/trigger](abi-testing.html#abi-sys-class-intel-pmt-crashlog-x-trigger)

## ABI file testing/sysfs-class-intel\_pmt-features

Has the following ABI:

* [/sys/class/intel\_pmt/features-<PCI BDF>/](abi-testing.html#abi-sys-class-intel-pmt-features-pci-bdf)

## ABI file testing/sysfs-class-iommu

Has the following ABI:

* [/sys/class/iommu/<iommu>/devices/](abi-testing.html#abi-sys-class-iommu-iommu-devices)
* [/sys/devices/.../iommu](abi-testing.html#abi-sys-devices-iommu)

## ABI file testing/sysfs-class-iommu-amd-iommu

Has the following ABI:

* [/sys/class/iommu/<iommu>/amd-iommu/cap](abi-testing.html#abi-sys-class-iommu-iommu-amd-iommu-cap)
* [/sys/class/iommu/<iommu>/amd-iommu/features](abi-testing.html#abi-sys-class-iommu-iommu-amd-iommu-features)

## ABI file testing/sysfs-class-iommu-intel-iommu

Has the following ABI:

* [/sys/class/iommu/<iommu>/intel-iommu/address](abi-testing.html#abi-sys-class-iommu-iommu-intel-iommu-address)
* [/sys/class/iommu/<iommu>/intel-iommu/cap](abi-testing.html#abi-sys-class-iommu-iommu-intel-iommu-cap)
* [/sys/class/iommu/<iommu>/intel-iommu/ecap](abi-testing.html#abi-sys-class-iommu-iommu-intel-iommu-ecap)
* [/sys/class/iommu/<iommu>/intel-iommu/version](abi-testing.html#abi-sys-class-iommu-iommu-intel-iommu-version)

## ABI file testing/sysfs-class-lcd

Has the following ABI:

* [/sys/class/lcd/<lcd>/lcd\_power](abi-testing.html#abi-sys-class-lcd-lcd-lcd-power)
* [/sys/class/lcd/<lcd>/contrast](abi-testing.html#abi-sys-class-lcd-lcd-contrast)
* [/sys/class/lcd/<lcd>/max\_contrast](abi-testing.html#abi-sys-class-lcd-lcd-max-contrast)

## ABI file testing/sysfs-class-led

Has the following ABI:

* [/sys/class/leds/<led>/brightness](abi-testing.html#abi-sys-class-leds-led-brightness)
* [/sys/class/leds/<led>/max\_brightness](abi-testing.html#abi-sys-class-leds-led-max-brightness)
* [/sys/class/leds/<led>/brightness\_hw\_changed](abi-testing.html#abi-sys-class-leds-led-brightness-hw-changed)
* [/sys/class/leds/<led>/trigger](abi-testing.html#abi-sys-class-leds-led-trigger)
* [/sys/class/leds/<led>/inverted](abi-testing.html#abi-sys-class-leds-led-inverted)

## ABI file testing/sysfs-class-led-driver-aw200xx

Has the following ABI:

* [/sys/class/leds/<led>/dim](abi-testing.html#abi-sys-class-leds-led-dim)

## ABI file testing/sysfs-class-led-driver-lm3533

Has the following ABI:

* [/sys/class/leds/<led>/als\_channel](abi-testing.html#abi-sys-class-leds-led-als-channel)
* [/sys/class/leds/<led>/als\_en](abi-testing.html#abi-sys-class-leds-led-als-en)
* [/sys/class/leds/<led>/falltime](abi-testing.html#abi-sys-class-leds-led-falltime)
* [/sys/class/leds/<led>/risetime](abi-testing.html#abi-sys-class-leds-led-falltime)
* [/sys/class/leds/<led>/id](abi-testing.html#abi-sys-class-leds-led-id)
* [/sys/class/leds/<led>/linear](abi-testing.html#abi-sys-class-leds-led-linear)
* [/sys/class/leds/<led>/pwm](abi-testing.html#abi-sys-class-leds-led-pwm)

## ABI file testing/sysfs-class-led-driver-turris-omnia

Has the following ABI:

* [/sys/class/leds/<led>/device/brightness](abi-testing.html#abi-sys-class-leds-led-device-brightness)
* [/sys/class/leds/<led>/device/gamma\_correction](abi-testing.html#abi-sys-class-leds-led-device-gamma-correction)

## ABI file testing/sysfs-class-led-flash

Has the following ABI:

* [/sys/class/leds/<led>/flash\_brightness](abi-testing.html#abi-sys-class-leds-led-flash-brightness)
* [/sys/class/leds/<led>/max\_flash\_brightness](abi-testing.html#abi-sys-class-leds-led-max-flash-brightness)
* [/sys/class/leds/<led>/flash\_timeout](abi-testing.html#abi-sys-class-leds-led-flash-timeout)
* [/sys/class/leds/<led>/max\_flash\_timeout](abi-testing.html#abi-sys-class-leds-led-max-flash-timeout)
* [/sys/class/leds/<led>/flash\_strobe](abi-testing.html#abi-sys-class-leds-led-flash-strobe)
* [/sys/class/leds/<led>/flash\_fault](abi-testing.html#abi-sys-class-leds-led-flash-fault)

## ABI file testing/sysfs-class-led-multicolor

Has the following ABI:

* [/sys/class/leds/<led>/multi\_index](abi-testing.html#abi-sys-class-leds-led-multi-index)
* [/sys/class/leds/<led>/multi\_intensity](abi-testing.html#abi-sys-class-leds-led-multi-intensity)

## ABI file testing/sysfs-class-led-trigger-netdev

Has the following ABI:

* [/sys/class/leds/<led>/device\_name](abi-testing.html#abi-sys-class-leds-led-device-name)
* [/sys/class/leds/<led>/interval](abi-testing.html#abi-sys-class-leds-led-interval)
* [/sys/class/leds/<led>/link](abi-testing.html#abi-sys-class-leds-led-link)
* [/sys/class/leds/<led>/tx](abi-testing.html#abi-sys-class-leds-led-tx)
* [/sys/class/leds/<led>/rx](abi-testing.html#abi-sys-class-leds-led-rx)
* [/sys/class/leds/<led>/offloaded](abi-testing.html#abi-sys-class-leds-led-offloaded)
* [/sys/class/leds/<led>/link\_10](abi-testing.html#abi-sys-class-leds-led-link-10)
* [/sys/class/leds/<led>/link\_100](abi-testing.html#abi-sys-class-leds-led-link-100)
* [/sys/class/leds/<led>/link\_1000](abi-testing.html#abi-sys-class-leds-led-link-1000)
* [/sys/class/leds/<led>/link\_2500](abi-testing.html#abi-sys-class-leds-led-link-2500)
* [/sys/class/leds/<led>/link\_5000](abi-testing.html#abi-sys-class-leds-led-link-5000)
* [/sys/class/leds/<led>/link\_10000](abi-testing.html#abi-sys-class-leds-led-link-10000)
* [/sys/class/leds/<led>/half\_duplex](abi-testing.html#abi-sys-class-leds-led-half-duplex)
* [/sys/class/leds/<led>/full\_duplex](abi-testing.html#abi-sys-class-leds-led-full-duplex)

## ABI file testing/sysfs-class-led-trigger-oneshot

Has the following ABI:

* [/sys/class/leds/<led>/delay\_on](abi-testing.html#abi-sys-class-leds-led-delay-on)
* [/sys/class/leds/<led>/delay\_off](abi-testing.html#abi-sys-class-leds-led-delay-off)
* [/sys/class/leds/<led>/invert](abi-testing.html#abi-sys-class-leds-led-invert)
* [/sys/class/leds/<led>/shot](abi-testing.html#abi-sys-class-leds-led-shot)

## ABI file testing/sysfs-class-led-trigger-pattern

Has the following ABI:

* [/sys/class/leds/<led>/pattern](abi-testing.html#abi-sys-class-leds-led-pattern)
* [/sys/class/leds/<led>/hr\_pattern](abi-testing.html#abi-sys-class-leds-led-hr-pattern)
* [/sys/class/leds/<led>/hw\_pattern](abi-testing.html#abi-sys-class-leds-led-hw-pattern)
* [/sys/class/leds/<led>/repeat](abi-testing.html#abi-sys-class-leds-led-repeat)

## ABI file testing/sysfs-class-led-trigger-tty

Has the following ABI:

* [/sys/class/leds/<tty\_led>/ttyname](abi-testing.html#abi-sys-class-leds-tty-led-ttyname)
* [/sys/class/leds/<tty\_led>/rx](abi-testing.html#abi-sys-class-leds-tty-led-rx)
* [/sys/class/leds/<tty\_led>/tx](abi-testing.html#abi-sys-class-leds-tty-led-tx)
* [/sys/class/leds/<tty\_led>/cts](abi-testing.html#abi-sys-class-leds-tty-led-cts)
* [/sys/class/leds/<tty\_led>/dsr](abi-testing.html#abi-sys-class-leds-tty-led-dsr)
* [/sys/class/leds/<tty\_led>/dcd](abi-testing.html#abi-sys-class-leds-tty-led-dcd)
* [/sys/class/leds/<tty\_led>/rng](abi-testing.html#abi-sys-class-leds-tty-led-rng)

## ABI file testing/sysfs-class-led-trigger-usbport

Has the following ABI:

* [/sys/class/leds/<led>/ports/<port>](abi-testing.html#abi-sys-class-leds-led-ports-port)

## ABI file testing/sysfs-class-leds-gt683r

Has the following ABI:

* [/sys/class/leds/<led>/gt683r/mode](abi-testing.html#abi-sys-class-leds-led-gt683r-mode)

## ABI file testing/sysfs-class-mei

Has the following ABI:

* [/sys/class/mei/](abi-testing.html#abi-sys-class-mei)
* [/sys/class/mei/mei<N>/](abi-testing.html#abi-sys-class-mei-mei-n)
* [/sys/class/mei/mei<N>/fw\_status](abi-testing.html#abi-sys-class-mei-mei-n-fw-status)
* [/sys/class/mei/mei<N>/hbm\_ver](abi-testing.html#abi-sys-class-mei-mei-n-hbm-ver)
* [/sys/class/mei/mei<N>/hbm\_ver\_drv](abi-testing.html#abi-sys-class-mei-mei-n-hbm-ver-drv)
* [/sys/class/mei/mei<N>/tx\_queue\_limit](abi-testing.html#abi-sys-class-mei-mei-n-tx-queue-limit)
* [/sys/class/mei/mei<N>/fw\_ver](abi-testing.html#abi-sys-class-mei-mei-n-fw-ver)
* [/sys/class/mei/mei<N>/dev\_state](abi-testing.html#abi-sys-class-mei-mei-n-dev-state)
* [/sys/class/mei/mei<N>/trc](abi-testing.html#abi-sys-class-mei-mei-n-trc)
* [/sys/class/mei/mei<N>/kind](abi-testing.html#abi-sys-class-mei-mei-n-kind)

## ABI file testing/sysfs-class-mic

Has the following ABI:

* [/sys/class/mic/](abi-testing.html#abi-sys-class-mic)
* [/sys/class/mic/mic<X>](abi-testing.html#abi-sys-class-mic-mic-x)
* [/sys/class/mic/mic<X>/family](abi-testing.html#abi-sys-class-mic-mic-x-family)
* [/sys/class/mic/mic<X>/stepping](abi-testing.html#abi-sys-class-mic-mic-x-stepping)
* [/sys/class/mic/mic<X>/state](abi-testing.html#abi-sys-class-mic-mic-x-state)
* [/sys/class/mic/mic<X>/shutdown\_status](abi-testing.html#abi-sys-class-mic-mic-x-shutdown-status)
* [/sys/class/mic/mic<X>/cmdline](abi-testing.html#abi-sys-class-mic-mic-x-cmdline)
* [/sys/class/mic/mic<X>/firmware](abi-testing.html#abi-sys-class-mic-mic-x-firmware)
* [/sys/class/mic/mic<X>/ramdisk](abi-testing.html#abi-sys-class-mic-mic-x-ramdisk)
* [/sys/class/mic/mic<X>/bootmode](abi-testing.html#abi-sys-class-mic-mic-x-bootmode)
* [/sys/class/mic/mic<X>/log\_buf\_addr](abi-testing.html#abi-sys-class-mic-mic-x-log-buf-addr)
* [/sys/class/mic/mic<X>/log\_buf\_len](abi-testing.html#abi-sys-class-mic-mic-x-log-buf-len)
* [/sys/class/mic/mic<X>/heartbeat\_enable](abi-testing.html#abi-sys-class-mic-mic-x-heartbeat-enable)

## ABI file testing/sysfs-class-mtd

Has the following ABI:

* [/sys/class/mtd/](abi-testing.html#abi-sys-class-mtd)
* [/sys/class/mtd/mtdX/](abi-testing.html#abi-sys-class-mtd-mtdx)
* [/sys/class/mtd/mtdXro/](abi-testing.html#abi-sys-class-mtd-mtdxro)
* [/sys/class/mtd/mtdX/dev](abi-testing.html#abi-sys-class-mtd-mtdx-dev)
* [/sys/class/mtd/mtdXro/dev](abi-testing.html#abi-sys-class-mtd-mtdxro-dev)
* [/sys/class/mtd/mtdX/erasesize](abi-testing.html#abi-sys-class-mtd-mtdx-erasesize)
* [/sys/class/mtd/mtdX/flags](abi-testing.html#abi-sys-class-mtd-mtdx-flags)
* [/sys/class/mtd/mtdX/name](abi-testing.html#abi-sys-class-mtd-mtdx-name)
* [/sys/class/mtd/mtdX/numeraseregions](abi-testing.html#abi-sys-class-mtd-mtdx-numeraseregions)
* [/sys/class/mtd/mtdX/oobsize](abi-testing.html#abi-sys-class-mtd-mtdx-oobsize)
* [/sys/class/mtd/mtdX/size](abi-testing.html#abi-sys-class-mtd-mtdx-size)
* [/sys/class/mtd/mtdX/type](abi-testing.html#abi-sys-class-mtd-mtdx-type)
* [/sys/class/mtd/mtdX/writesize](abi-testing.html#abi-sys-class-mtd-mtdx-writesize)
* [/sys/class/mtd/mtdX/ecc\_strength](abi-testing.html#abi-sys-class-mtd-mtdx-ecc-strength)
* [/sys/class/mtd/mtdX/bitflip\_threshold](abi-testing.html#abi-sys-class-mtd-mtdx-bitflip-threshold)
* [/sys/class/mtd/mtdX/ecc\_step\_size](abi-testing.html#abi-sys-class-mtd-mtdx-ecc-step-size)
* [/sys/class/mtd/mtdX/ecc\_failures](abi-testing.html#abi-sys-class-mtd-mtdx-ecc-failures)
* [/sys/class/mtd/mtdX/corrected\_bits](abi-testing.html#abi-sys-class-mtd-mtdx-corrected-bits)
* [/sys/class/mtd/mtdX/bad\_blocks](abi-testing.html#abi-sys-class-mtd-mtdx-bad-blocks)
* [/sys/class/mtd/mtdX/bbt\_blocks](abi-testing.html#abi-sys-class-mtd-mtdx-bbt-blocks)
* [/sys/class/mtd/mtdX/offset](abi-testing.html#abi-sys-class-mtd-mtdx-offset)
* [/sys/class/mtd/mtdX/oobavail](abi-testing.html#abi-sys-class-mtd-mtdx-oobavail)

## ABI file testing/sysfs-class-mux

Has the following ABI:

* [/sys/class/mux/](abi-testing.html#abi-sys-class-mux)
* [/sys/class/mux/muxchip<N>/](abi-testing.html#abi-sys-class-mux-muxchip-n)

## ABI file testing/sysfs-class-net

Has the following ABI:

* [/sys/class/net/<iface>/name\_assign\_type](abi-testing.html#abi-sys-class-net-iface-name-assign-type)
* [/sys/class/net/<iface>/addr\_assign\_type](abi-testing.html#abi-sys-class-net-iface-addr-assign-type)
* [/sys/class/net/<iface>/addr\_len](abi-testing.html#abi-sys-class-net-iface-addr-len)
* [/sys/class/net/<iface>/address](abi-testing.html#abi-sys-class-net-iface-address)
* [/sys/class/net/<bridge iface>/bridge/group\_fwd\_mask](abi-testing.html#abi-sys-class-net-bridge-iface-bridge-group-fwd-mask)
* [/sys/class/net/<iface>/broadcast](abi-testing.html#abi-sys-class-net-iface-broadcast)
* [/sys/class/net/<iface>/carrier](abi-testing.html#abi-sys-class-net-iface-carrier)
* [/sys/class/net/<iface>/dev\_id](abi-testing.html#abi-sys-class-net-iface-dev-id)
* [/sys/class/net/<iface>/dev\_port](abi-testing.html#abi-sys-class-net-iface-dev-port)
* [/sys/class/net/<iface>/dormant](abi-testing.html#abi-sys-class-net-iface-dormant)
* [/sys/class/net/<iface>/testing](abi-testing.html#abi-sys-class-net-iface-testing)
* [/sys/class/net/<iface>/duplex](abi-testing.html#abi-sys-class-net-iface-duplex)
* [/sys/class/net/<iface>/flags](abi-testing.html#abi-sys-class-net-iface-flags)
* [/sys/class/net/<iface>/ifalias](abi-testing.html#abi-sys-class-net-iface-ifalias)
* [/sys/class/net/<iface>/ifindex](abi-testing.html#abi-sys-class-net-iface-ifindex)
* [/sys/class/net/<iface>/iflink](abi-testing.html#abi-sys-class-net-iface-iflink)
* [/sys/class/net/<iface>/link\_mode](abi-testing.html#abi-sys-class-net-iface-link-mode)
* [/sys/class/net/<iface>/mtu](abi-testing.html#abi-sys-class-net-iface-mtu)
* [/sys/class/net/<iface>/netdev\_group](abi-testing.html#abi-sys-class-net-iface-netdev-group)
* [/sys/class/net/<iface>/operstate](abi-testing.html#abi-sys-class-net-iface-operstate)
* [/sys/class/net/<iface>/phys\_port\_id](abi-testing.html#abi-sys-class-net-iface-phys-port-id)
* [/sys/class/net/<iface>/phys\_port\_name](abi-testing.html#abi-sys-class-net-iface-phys-port-name)
* [/sys/class/net/<iface>/speed](abi-testing.html#abi-sys-class-net-iface-speed)
* [/sys/class/net/<iface>/tx\_queue\_len](abi-testing.html#abi-sys-class-net-iface-tx-queue-len)
* [/sys/class/net/<iface>/type](abi-testing.html#abi-sys-class-net-iface-type)
* [/sys/class/net/<iface>/phys\_switch\_id](abi-testing.html#abi-sys-class-net-iface-phys-switch-id)
* [/sys/class/net/<iface>/phydev](abi-testing.html#abi-sys-class-net-iface-phydev)
* [/sys/class/net/<iface>/carrier\_changes](abi-testing.html#abi-sys-class-net-iface-carrier-changes)
* [/sys/class/net/<iface>/carrier\_up\_count](abi-testing.html#abi-sys-class-net-iface-carrier-up-count)
* [/sys/class/net/<iface>/carrier\_down\_count](abi-testing.html#abi-sys-class-net-iface-carrier-down-count)
* [/sys/class/net/<iface>/threaded](abi-testing.html#abi-sys-class-net-iface-threaded)

## ABI file testing/sysfs-class-net-cdc\_ncm

Has the following ABI:

* [/sys/class/net/<iface>/cdc\_ncm/min\_tx\_pkt](abi-testing.html#abi-sys-class-net-iface-cdc-ncm-min-tx-pkt)
* [/sys/class/net/<iface>/cdc\_ncm/ndp\_to\_end](abi-testing.html#abi-sys-class-net-iface-cdc-ncm-ndp-to-end)
* [/sys/class/net/<iface>/cdc\_ncm/rx\_max](abi-testing.html#abi-sys-class-net-iface-cdc-ncm-rx-max)
* [/sys/class/net/<iface>/cdc\_ncm/tx\_max](abi-testing.html#abi-sys-class-net-iface-cdc-ncm-tx-max)
* [/sys/class/net/<iface>/cdc\_ncm/tx\_timer\_usecs](abi-testing.html#abi-sys-class-net-iface-cdc-ncm-tx-timer-usecs)
* [/sys/class/net/<iface>/cdc\_ncm/bmNtbFormatsSupported](abi-testing.html#abi-sys-class-net-iface-cdc-ncm-bmntbformatssupported)
* [/sys/class/net/<iface>/cdc\_ncm/dwNtbInMaxSize](abi-testing.html#abi-sys-class-net-iface-cdc-ncm-dwntbinmaxsize)
* [/sys/class/net/<iface>/cdc\_ncm/wNdpInDivisor](abi-testing.html#abi-sys-class-net-iface-cdc-ncm-wndpindivisor)
* [/sys/class/net/<iface>/cdc\_ncm/wNdpInPayloadRemainder](abi-testing.html#abi-sys-class-net-iface-cdc-ncm-wndpinpayloadremainder)
* [/sys/class/net/<iface>/cdc\_ncm/wNdpInAlignment](abi-testing.html#abi-sys-class-net-iface-cdc-ncm-wndpinalignment)
* [/sys/class/net/<iface>/cdc\_ncm/dwNtbOutMaxSize](abi-testing.html#abi-sys-class-net-iface-cdc-ncm-dwntboutmaxsize)
* [/sys/class/net/<iface>/cdc\_ncm/wNdpOutDivisor](abi-testing.html#abi-sys-class-net-iface-cdc-ncm-wndpoutdivisor)
* [/sys/class/net/<iface>/cdc\_ncm/wNdpOutPayloadRemainder](abi-testing.html#abi-sys-class-net-iface-cdc-ncm-wndpoutpayloadremainder)
* [/sys/class/net/<iface>/cdc\_ncm/wNdpOutAlignment](abi-testing.html#abi-sys-class-net-iface-cdc-ncm-wndpoutalignment)
* [/sys/class/net/<iface>/cdc\_ncm/wNtbOutMaxDatagrams](abi-testing.html#abi-sys-class-net-iface-cdc-ncm-wntboutmaxdatagrams)

## ABI file testing/sysfs-class-net-dsa

Has the following ABI:

* [/sys/class/net/<iface>/dsa/tagging](abi-testing.html#abi-sys-class-net-iface-dsa-tagging)

## ABI file testing/sysfs-class-net-grcan

Has the following ABI:

* [/sys/class/net/<iface>/grcan/enable0](abi-testing.html#abi-sys-class-net-iface-grcan-enable0)
* [/sys/class/net/<iface>/grcan/enable1](abi-testing.html#abi-sys-class-net-iface-grcan-enable1)
* [/sys/class/net/<iface>/grcan/select](abi-testing.html#abi-sys-class-net-iface-grcan-select)

## ABI file testing/sysfs-class-net-janz-ican3

Has the following ABI:

* [/sys/class/net/<iface>/termination](abi-testing.html#abi-sys-class-net-iface-termination)
* [/sys/class/net/<iface>/fwinfo](abi-testing.html#abi-sys-class-net-iface-fwinfo)

## ABI file testing/sysfs-class-net-peak\_usb

Has the following ABI:

* [/sys/class/net/<iface>/peak\_usb/can\_channel\_id](abi-testing.html#abi-sys-class-net-iface-peak-usb-can-channel-id)

## ABI file testing/sysfs-class-net-phydev

Has the following ABI:

* [/sys/class/mdio\_bus/<bus>/<device>/attached\_dev](abi-testing.html#abi-sys-class-mdio-bus-bus-device-attached-dev)
* [/sys/class/mdio\_bus/<bus>/<device>/phy\_has\_fixups](abi-testing.html#abi-sys-class-mdio-bus-bus-device-phy-has-fixups)
* [/sys/class/mdio\_bus/<bus>/<device>/phy\_id](abi-testing.html#abi-sys-class-mdio-bus-bus-device-phy-id)
* [/sys/class/mdio\_bus/<bus>/<device>/c45\_phy\_ids/mmd<n>\_device\_id](abi-testing.html#abi-sys-class-mdio-bus-bus-device-c45-phy-ids-mmd-n-device-id)
* [/sys/class/mdio\_bus/<bus>/<device>/phy\_interface](abi-testing.html#abi-sys-class-mdio-bus-bus-device-phy-interface)
* [/sys/class/mdio\_bus/<bus>/<device>/phy\_standalone](abi-testing.html#abi-sys-class-mdio-bus-bus-device-phy-standalone)
* [/sys/class/mdio\_bus/<bus>/<device>/phy\_dev\_flags](abi-testing.html#abi-sys-class-mdio-bus-bus-device-phy-dev-flags)

## ABI file testing/sysfs-class-net-qmi

Has the following ABI:

* [/sys/class/net/<iface>/qmi/raw\_ip](abi-testing.html#abi-sys-class-net-iface-qmi-raw-ip)
* [/sys/class/net/<iface>/qmi/add\_mux](abi-testing.html#abi-sys-class-net-iface-qmi-add-mux)
* [/sys/class/net/<iface>/qmi/del\_mux](abi-testing.html#abi-sys-class-net-iface-qmi-del-mux)
* [/sys/class/net/<qmimux iface>/qmap/mux\_id](abi-testing.html#abi-sys-class-net-qmimux-iface-qmap-mux-id)
* [/sys/class/net/<iface>/qmi/pass\_through](abi-testing.html#abi-sys-class-net-iface-qmi-pass-through)

## ABI file testing/sysfs-class-net-queues

Has the following ABI:

* [/sys/class/net/<iface>/queues/rx-<queue>/rps\_cpus](abi-testing.html#abi-sys-class-net-iface-queues-rx-queue-rps-cpus)
* [/sys/class/net/<iface>/queues/rx-<queue>/rps\_flow\_cnt](abi-testing.html#abi-sys-class-net-iface-queues-rx-queue-rps-flow-cnt)
* [/sys/class/net/<iface>/queues/tx-<queue>/tx\_timeout](abi-testing.html#abi-sys-class-net-iface-queues-tx-queue-tx-timeout)
* [/sys/class/net/<iface>/queues/tx-<queue>/tx\_maxrate](abi-testing.html#abi-sys-class-net-iface-queues-tx-queue-tx-maxrate)
* [/sys/class/net/<iface>/queues/tx-<queue>/xps\_cpus](abi-testing.html#abi-sys-class-net-iface-queues-tx-queue-xps-cpus)
* [/sys/class/net/<iface>/queues/tx-<queue>/xps\_rxqs](abi-testing.html#abi-sys-class-net-iface-queues-tx-queue-xps-rxqs)
* [/sys/class/net/<iface>/queues/tx-<queue>/byte\_queue\_limits/hold\_time](abi-testing.html#abi-sys-class-net-iface-queues-tx-queue-byte-queue-limits-hold-time)
* [/sys/class/net/<iface>/queues/tx-<queue>/byte\_queue\_limits/inflight](abi-testing.html#abi-sys-class-net-iface-queues-tx-queue-byte-queue-limits-inflight)
* [/sys/class/net/<iface>/queues/tx-<queue>/byte\_queue\_limits/limit](abi-testing.html#abi-sys-class-net-iface-queues-tx-queue-byte-queue-limits-limit)
* [/sys/class/net/<iface>/queues/tx-<queue>/byte\_queue\_limits/limit\_max](abi-testing.html#abi-sys-class-net-iface-queues-tx-queue-byte-queue-limits-limit-max)
* [/sys/class/net/<iface>/queues/tx-<queue>/byte\_queue\_limits/limit\_min](abi-testing.html#abi-sys-class-net-iface-queues-tx-queue-byte-queue-limits-limit-min)
* [/sys/class/net/<iface>/queues/tx-<queue>/byte\_queue\_limits/stall\_thrs](abi-testing.html#abi-sys-class-net-iface-queues-tx-queue-byte-queue-limits-stall-thrs)
* [/sys/class/net/<iface>/queues/tx-<queue>/byte\_queue\_limits/stall\_cnt](abi-testing.html#abi-sys-class-net-iface-queues-tx-queue-byte-queue-limits-stall-cnt)
* [/sys/class/net/<iface>/queues/tx-<queue>/byte\_queue\_limits/stall\_max](abi-testing.html#abi-sys-class-net-iface-queues-tx-queue-byte-queue-limits-stall-max)

## ABI file testing/sysfs-class-net-statistics

Has the following ABI:

* [/sys/class/net/<iface>/statistics/collisions](abi-testing.html#abi-sys-class-net-iface-statistics-collisions)
* [/sys/class/net/<iface>/statistics/multicast](abi-testing.html#abi-sys-class-net-iface-statistics-multicast)
* [/sys/class/net/<iface>/statistics/rx\_bytes](abi-testing.html#abi-sys-class-net-iface-statistics-rx-bytes)
* [/sys/class/net/<iface>/statistics/rx\_compressed](abi-testing.html#abi-sys-class-net-iface-statistics-rx-compressed)
* [/sys/class/net/<iface>/statistics/rx\_crc\_errors](abi-testing.html#abi-sys-class-net-iface-statistics-rx-crc-errors)
* [/sys/class/net/<iface>/statistics/rx\_dropped](abi-testing.html#abi-sys-class-net-iface-statistics-rx-dropped)
* [/sys/class/net/<iface>/statistics/rx\_errors](abi-testing.html#abi-sys-class-net-iface-statistics-rx-errors)
* [/sys/class/net/<iface>/statistics/rx\_fifo\_errors](abi-testing.html#abi-sys-class-net-iface-statistics-rx-fifo-errors)
* [/sys/class/net/<iface>/statistics/rx\_frame\_errors](abi-testing.html#abi-sys-class-net-iface-statistics-rx-frame-errors)
* [/sys/class/net/<iface>/statistics/rx\_length\_errors](abi-testing.html#abi-sys-class-net-iface-statistics-rx-length-errors)
* [/sys/class/net/<iface>/statistics/rx\_missed\_errors](abi-testing.html#abi-sys-class-net-iface-statistics-rx-missed-errors)
* [/sys/class/net/<iface>/statistics/rx\_nohandler](abi-testing.html#abi-sys-class-net-iface-statistics-rx-nohandler)
* [/sys/class/net/<iface>/statistics/rx\_over\_errors](abi-testing.html#abi-sys-class-net-iface-statistics-rx-over-errors)
* [/sys/class/net/<iface>/statistics/rx\_packets](abi-testing.html#abi-sys-class-net-iface-statistics-rx-packets)
* [/sys/class/net/<iface>/statistics/tx\_aborted\_errors](abi-testing.html#abi-sys-class-net-iface-statistics-tx-aborted-errors)
* [/sys/class/net/<iface>/statistics/tx\_bytes](abi-testing.html#abi-sys-class-net-iface-statistics-tx-bytes)
* [/sys/class/net/<iface>/statistics/tx\_carrier\_errors](abi-testing.html#abi-sys-class-net-iface-statistics-tx-carrier-errors)
* [/sys/class/net/<iface>/statistics/tx\_compressed](abi-testing.html#abi-sys-class-net-iface-statistics-tx-compressed)
* [/sys/class/net/<iface>/statistics/tx\_dropped](abi-testing.html#abi-sys-class-net-iface-statistics-tx-dropped)
* [/sys/class/net/<iface>/statistics/tx\_errors](abi-testing.html#abi-sys-class-net-iface-statistics-tx-errors)
* [/sys/class/net/<iface>/statistics/tx\_fifo\_errors](abi-testing.html#abi-sys-class-net-iface-statistics-tx-fifo-errors)
* [/sys/class/net/<iface>/statistics/tx\_heartbeat\_errors](abi-testing.html#abi-sys-class-net-iface-statistics-tx-heartbeat-errors)
* [/sys/class/net/<iface>/statistics/tx\_packets](abi-testing.html#abi-sys-class-net-iface-statistics-tx-packets)
* [/sys/class/net/<iface>/statistics/tx\_window\_errors](abi-testing.html#abi-sys-class-net-iface-statistics-tx-window-errors)

## ABI file testing/sysfs-class-ocxl

Has the following ABI:

* [/sys/class/ocxl/<afu name>/afu\_version](abi-testing.html#abi-sys-class-ocxl-afu-name-afu-version)
* [/sys/class/ocxl/<afu name>/contexts](abi-testing.html#abi-sys-class-ocxl-afu-name-contexts)
* [/sys/class/ocxl/<afu name>/pp\_mmio\_size](abi-testing.html#abi-sys-class-ocxl-afu-name-pp-mmio-size)
* [/sys/class/ocxl/<afu name>/global\_mmio\_size](abi-testing.html#abi-sys-class-ocxl-afu-name-global-mmio-size)
* [/sys/class/ocxl/<afu name>/global\_mmio\_area](abi-testing.html#abi-sys-class-ocxl-afu-name-global-mmio-area)
* [/sys/class/ocxl/<afu name>/reload\_on\_reset](abi-testing.html#abi-sys-class-ocxl-afu-name-reload-on-reset)

## ABI file testing/sysfs-class-platform-profile

Has the following ABI:

* [/sys/class/platform-profile/platform-profile-X/name](abi-testing.html#abi-sys-class-platform-profile-platform-profile-x-name)
* [/sys/class/platform-profile/platform-profile-X/choices](abi-testing.html#abi-sys-class-platform-profile-platform-profile-x-choices)
* [/sys/class/platform-profile/platform-profile-X/profile](abi-testing.html#abi-sys-class-platform-profile-platform-profile-x-profile)

## ABI file testing/sysfs-class-power

**General Properties**

Has the following ABI:

* [/sys/class/power\_supply/<supply\_name>/manufacturer](abi-testing.html#abi-sys-class-power-supply-supply-name-manufacturer)
* [/sys/class/power\_supply/<supply\_name>/model\_name](abi-testing.html#abi-sys-class-power-supply-supply-name-model-name)
* [/sys/class/power\_supply/<supply\_name>/serial\_number](abi-testing.html#abi-sys-class-power-supply-supply-name-serial-number)
* [/sys/class/power\_supply/<supply\_name>/type](abi-testing.html#abi-sys-class-power-supply-supply-name-type)
* [/sys/class/power\_supply/<supply\_name>/current\_avg](abi-testing.html#abi-sys-class-power-supply-supply-name-current-avg)
* [/sys/class/power\_supply/<supply\_name>/current\_max](abi-testing.html#abi-sys-class-power-supply-supply-name-current-max)
* [/sys/class/power\_supply/<supply\_name>/current\_now](abi-testing.html#abi-sys-class-power-supply-supply-name-current-now)
* [/sys/class/power\_supply/<supply\_name>/temp](abi-testing.html#abi-sys-class-power-supply-supply-name-temp)
* [/sys/class/power\_supply/<supply\_name>/temp\_alert\_max](abi-testing.html#abi-sys-class-power-supply-supply-name-temp-alert-max)
* [/sys/class/power\_supply/<supply\_name>/temp\_alert\_min](abi-testing.html#abi-sys-class-power-supply-supply-name-temp-alert-min)
* [/sys/class/power\_supply/<supply\_name>/temp\_max](abi-testing.html#abi-sys-class-power-supply-supply-name-temp-max)
* [/sys/class/power\_supply/<supply\_name>/temp\_min](abi-testing.html#abi-sys-class-power-supply-supply-name-temp-min)
* [/sys/class/power\_supply/<supply\_name>/voltage\_max,](abi-testing.html#abi-sys-class-power-supply-supply-name-voltage-max)
* [/sys/class/power\_supply/<supply\_name>/voltage\_min,](abi-testing.html#abi-sys-class-power-supply-supply-name-voltage-min)
* [/sys/class/power\_supply/<supply\_name>/voltage\_now,](abi-testing.html#abi-sys-class-power-supply-supply-name-voltage-now)
* [/sys/class/power\_supply/<supply\_name>/capacity](abi-testing.html#abi-sys-class-power-supply-supply-name-capacity)
* [/sys/class/power\_supply/<supply\_name>/capacity\_alert\_max](abi-testing.html#abi-sys-class-power-supply-supply-name-capacity-alert-max)
* [/sys/class/power\_supply/<supply\_name>/capacity\_alert\_min](abi-testing.html#abi-sys-class-power-supply-supply-name-capacity-alert-min)
* [/sys/class/power\_supply/<supply\_name>/capacity\_error\_margin](abi-testing.html#abi-sys-class-power-supply-supply-name-capacity-error-margin)
* [/sys/class/power\_supply/<supply\_name>/capacity\_level](abi-testing.html#abi-sys-class-power-supply-supply-name-capacity-level)
* [/sys/class/power\_supply/<supply\_name>/charge\_control\_limit](abi-testing.html#abi-sys-class-power-supply-supply-name-charge-control-limit)
* [/sys/class/power\_supply/<supply\_name>/charge\_control\_limit\_max](abi-testing.html#abi-sys-class-power-supply-supply-name-charge-control-limit-max)
* [/sys/class/power\_supply/<supply\_name>/charge\_control\_start\_threshold](abi-testing.html#abi-sys-class-power-supply-supply-name-charge-control-start-threshold)
* [/sys/class/power\_supply/<supply\_name>/charge\_control\_end\_threshold](abi-testing.html#abi-sys-class-power-supply-supply-name-charge-control-end-threshold)
* [/sys/class/power\_supply/<supply\_name>/charge\_type](abi-testing.html#abi-sys-class-power-supply-supply-name-charge-type)
* [/sys/class/power\_supply/<supply\_name>/charge\_types](abi-testing.html#abi-sys-class-power-supply-supply-name-charge-types)
* [/sys/class/power\_supply/<supply\_name>/charge\_term\_current](abi-testing.html#abi-sys-class-power-supply-supply-name-charge-term-current)
* [/sys/class/power\_supply/<supply\_name>/health](abi-testing.html#abi-sys-class-power-supply-supply-name-health)
* [/sys/class/power\_supply/<supply\_name>/precharge\_current](abi-testing.html#abi-sys-class-power-supply-supply-name-precharge-current)
* [/sys/class/power\_supply/<supply\_name>/present](abi-testing.html#abi-sys-class-power-supply-supply-name-present)
* [/sys/class/power\_supply/<supply\_name>/status](abi-testing.html#abi-sys-class-power-supply-supply-name-status)
* [/sys/class/power\_supply/<supply\_name>/charge\_behaviour](abi-testing.html#abi-sys-class-power-supply-supply-name-charge-behaviour)
* [/sys/class/power\_supply/<supply\_name>/technology](abi-testing.html#abi-sys-class-power-supply-supply-name-technology)
* [/sys/class/power\_supply/<supply\_name>/voltage\_avg,](abi-testing.html#abi-sys-class-power-supply-supply-name-voltage-avg)
* [/sys/class/power\_supply/<supply\_name>/cycle\_count](abi-testing.html#abi-sys-class-power-supply-supply-name-cycle-count)
* [/sys/class/power\_supply/<supply\_name>/internal\_resistance](abi-testing.html#abi-sys-class-power-supply-supply-name-internal-resistance)
* [/sys/class/power\_supply/<supply\_name>/state\_of\_health](abi-testing.html#abi-sys-class-power-supply-supply-name-state-of-health)
* [/sys/class/power\_supply/<supply\_name>/input\_current\_limit](abi-testing.html#abi-sys-class-power-supply-supply-name-input-current-limit)
* [/sys/class/power\_supply/<supply\_name>/input\_voltage\_limit](abi-testing.html#abi-sys-class-power-supply-supply-name-input-voltage-limit)
* [/sys/class/power\_supply/<supply\_name>/input\_power\_limit](abi-testing.html#abi-sys-class-power-supply-supply-name-input-power-limit)
* [/sys/class/power\_supply/<supply\_name>/online,](abi-testing.html#abi-sys-class-power-supply-supply-name-online)
* [/sys/class/power\_supply/<supply\_name>/usb\_type](abi-testing.html#abi-sys-class-power-supply-supply-name-usb-type)
* [/sys/class/power/ds2760-battery.\*/charge\_now](abi-testing.html#abi-sys-class-power-ds2760-battery-charge-now)
* [/sys/class/power/ds2760-battery.\*/charge\_full](abi-testing.html#abi-sys-class-power-ds2760-battery-charge-full)
* [/sys/class/power\_supply/max14577-charger/device/fast\_charge\_timer](abi-testing.html#abi-sys-class-power-supply-max14577-charger-device-fast-charge-timer)
* [/sys/class/power\_supply/max77693-charger/device/fast\_charge\_timer](abi-testing.html#abi-sys-class-power-supply-max77693-charger-device-fast-charge-timer)
* [/sys/class/power\_supply/max77693-charger/device/top\_off\_threshold\_current](abi-testing.html#abi-sys-class-power-supply-max77693-charger-device-top-off-threshold-current)
* [/sys/class/power\_supply/max77693-charger/device/top\_off\_timer](abi-testing.html#abi-sys-class-power-supply-max77693-charger-device-top-off-timer)
* [/sys/class/power\_supply/bq24257-charger/ovp\_voltage](abi-testing.html#abi-sys-class-power-supply-bq24257-charger-ovp-voltage)
* [/sys/class/power\_supply/bq24257-charger/in\_dpm\_voltage](abi-testing.html#abi-sys-class-power-supply-bq24257-charger-in-dpm-voltage)
* [/sys/class/power\_supply/bq24257-charger/high\_impedance\_enable](abi-testing.html#abi-sys-class-power-supply-bq24257-charger-high-impedance-enable)
* [/sys/class/power\_supply/bq24257-charger/sysoff\_enable](abi-testing.html#abi-sys-class-power-supply-bq24257-charger-sysoff-enable)
* [/sys/class/power\_supply/<supply\_name>/manufacture\_year](abi-testing.html#abi-sys-class-power-supply-supply-name-manufacture-year)
* [/sys/class/power\_supply/<supply\_name>/manufacture\_month](abi-testing.html#abi-sys-class-power-supply-supply-name-manufacture-month)
* [/sys/class/power\_supply/<supply\_name>/manufacture\_day](abi-testing.html#abi-sys-class-power-supply-supply-name-manufacture-day)
* [/sys/class/power\_supply/<supply\_name>/extensions/<extension\_name>](abi-testing.html#abi-sys-class-power-supply-supply-name-extensions-extension-name)
* [/sys/class/power\_supply/max8971-charger/fast\_charge\_timer](abi-testing.html#abi-sys-class-power-supply-max8971-charger-fast-charge-timer)
* [/sys/class/power\_supply/max8971-charger/top\_off\_threshold\_current](abi-testing.html#abi-sys-class-power-supply-max8971-charger-top-off-threshold-current)
* [/sys/class/power\_supply/max8971-charger/top\_off\_timer](abi-testing.html#abi-sys-class-power-supply-max8971-charger-top-off-timer)

## ABI file testing/sysfs-class-power-gaokun

Has the following ABI:

* [/sys/class/power\_supply/gaokun-ec-battery/smart\_charge\_delay](abi-testing.html#abi-sys-class-power-supply-gaokun-ec-battery-smart-charge-delay)
* [/sys/class/power\_supply/gaokun-ec-battery/battery\_adaptive\_charge](abi-testing.html#abi-sys-class-power-supply-gaokun-ec-battery-battery-adaptive-charge)

## ABI file testing/sysfs-class-power-ltc4162l

Has the following ABI:

* [/sys/class/power\_supply/ltc4162-l/charge\_status](abi-testing.html#abi-sys-class-power-supply-ltc4162-l-charge-status)
* [/sys/class/power\_supply/ltc4162-l/ibat](abi-testing.html#abi-sys-class-power-supply-ltc4162-l-ibat)
* [/sys/class/power\_supply/ltc4162-l/vbat](abi-testing.html#abi-sys-class-power-supply-ltc4162-l-vbat)
* [/sys/class/power\_supply/ltc4162-l/vbat\_avg](abi-testing.html#abi-sys-class-power-supply-ltc4162-l-vbat-avg)
* [/sys/class/power\_supply/ltc4162-l/force\_telemetry](abi-testing.html#abi-sys-class-power-supply-ltc4162-l-force-telemetry)
* [/sys/class/power\_supply/ltc4162-l/arm\_ship\_mode](abi-testing.html#abi-sys-class-power-supply-ltc4162-l-arm-ship-mode)

## ABI file testing/sysfs-class-power-max1720x

Has the following ABI:

* [/sys/class/power\_supply/max1720x/temp\_ain1](abi-testing.html#abi-sys-class-power-supply-max1720x-temp-ain1)
* [/sys/class/power\_supply/max1720x/temp\_ain2](abi-testing.html#abi-sys-class-power-supply-max1720x-temp-ain2)
* [/sys/class/power\_supply/max1720x/temp\_int](abi-testing.html#abi-sys-class-power-supply-max1720x-temp-int)

## ABI file testing/sysfs-class-power-mp2629

Has the following ABI:

* [/sys/class/power\_supply/mp2629\_battery/batt\_impedance\_compen](abi-testing.html#abi-sys-class-power-supply-mp2629-battery-batt-impedance-compen)

## ABI file testing/sysfs-class-power-rt9467

Has the following ABI:

* [/sys/class/power\_supply/rt9467-\*/sysoff\_enable](abi-testing.html#abi-sys-class-power-supply-rt9467-sysoff-enable)

## ABI file testing/sysfs-class-power-rt9471

Has the following ABI:

* [/sys/class/power\_supply/rt9471-\*/sysoff\_enable](abi-testing.html#abi-sys-class-power-supply-rt9471-sysoff-enable)
* [/sys/class/power\_supply/rt9471-\*/port\_detect\_enable](abi-testing.html#abi-sys-class-power-supply-rt9471-port-detect-enable)

## ABI file testing/sysfs-class-power-rt9756

Has the following ABI:

* [/sys/class/power\_supply/rt9756-\*/watchdog\_timer](abi-testing.html#abi-sys-class-power-supply-rt9756-watchdog-timer)
* [/sys/class/power\_supply/rt9756-\*/operation\_mode](abi-testing.html#abi-sys-class-power-supply-rt9756-operation-mode)

## ABI file testing/sysfs-class-power-surface

Has the following ABI:

* [/sys/class/power\_supply/<supply\_name>/alarm](abi-testing.html#abi-sys-class-power-supply-supply-name-alarm)

## ABI file testing/sysfs-class-power-twl4030

Has the following ABI:

* [/sys/class/power\_supply/twl4030\_usb/mode](abi-testing.html#abi-sys-class-power-supply-twl4030-usb-mode)
* [/sys/class/power\_supply/twl4030\_ac/mode](abi-testing.html#abi-sys-class-power-supply-twl4030-ac-mode)

## ABI file testing/sysfs-class-power-wilco

Has the following ABI:

* [/sys/class/power\_supply/wilco-charger/charge\_type](abi-testing.html#abi-sys-class-power-supply-wilco-charger-charge-type)
* [/sys/class/power\_supply/wilco-charger/charge\_control\_start\_threshold](abi-testing.html#abi-sys-class-power-supply-wilco-charger-charge-control-start-threshold)
* [/sys/class/power\_supply/wilco-charger/charge\_control\_end\_threshold](abi-testing.html#abi-sys-class-power-supply-wilco-charger-charge-control-end-threshold)

## ABI file testing/sysfs-class-powercap

Has the following ABI:

* [/sys/class/powercap/](abi-testing.html#abi-sys-class-powercap)
* [/sys/class/powercap/<control type>](abi-testing.html#abi-sys-class-powercap-control-type)
* [/sys/class/powercap/<control type>/enabled](abi-testing.html#abi-sys-class-powercap-control-type-enabled)
* [/sys/class/powercap/<control type>/<power zone>](abi-testing.html#abi-sys-class-powercap-control-type-power-zone)
* [/sys/class/powercap/<control type>/<power zone>/<child power zone>](abi-testing.html#abi-sys-class-powercap-control-type-power-zone-child-power-zone)
* [/sys/class/powercap/.../<power zone>/name](abi-testing.html#abi-sys-class-powercap-power-zone-name)
* [/sys/class/powercap/.../<power zone>/energy\_uj](abi-testing.html#abi-sys-class-powercap-power-zone-energy-uj)
* [/sys/class/powercap/.../<power zone>/max\_energy\_range\_uj](abi-testing.html#abi-sys-class-powercap-power-zone-max-energy-range-uj)
* [/sys/class/powercap/.../<power zone>/power\_uw](abi-testing.html#abi-sys-class-powercap-power-zone-power-uw)
* [/sys/class/powercap/.../<power zone>/max\_power\_range\_uw](abi-testing.html#abi-sys-class-powercap-power-zone-max-power-range-uw)
* [/sys/class/powercap/.../<power zone>/constraint\_X\_name](abi-testing.html#abi-sys-class-powercap-power-zone-constraint-x-name)
* [/sys/class/powercap/.../<power zone>/constraint\_X\_power\_limit\_uw](abi-testing.html#abi-sys-class-powercap-power-zone-constraint-x-power-limit-uw)
* [/sys/class/powercap/.../<power zone>/constraint\_X\_time\_window\_us](abi-testing.html#abi-sys-class-powercap-power-zone-constraint-x-time-window-us)
* [/sys/class/powercap/<control type>/.../constraint\_X\_max\_power\_uw](abi-testing.html#abi-sys-class-powercap-control-type-constraint-x-max-power-uw)
* [/sys/class/powercap/<control type>/.../constraint\_X\_min\_power\_uw](abi-testing.html#abi-sys-class-powercap-control-type-constraint-x-min-power-uw)
* [/sys/class/powercap/.../<power zone>/constraint\_X\_max\_time\_window\_us](abi-testing.html#abi-sys-class-powercap-power-zone-constraint-x-max-time-window-us)
* [/sys/class/powercap/.../<power zone>/constraint\_X\_min\_time\_window\_us](abi-testing.html#abi-sys-class-powercap-power-zone-constraint-x-min-time-window-us)
* [/sys/class/powercap/.../<power zone>/enabled](abi-testing.html#abi-sys-class-powercap-power-zone-enabled)

## ABI file testing/sysfs-class-pwm

Has the following ABI:

* [/sys/class/pwm/](abi-testing.html#abi-sys-class-pwm)
* [/sys/class/pwm/pwmchip<N>/](abi-testing.html#abi-sys-class-pwm-pwmchip-n)
* [/sys/class/pwm/pwmchip<N>/npwm](abi-testing.html#abi-sys-class-pwm-pwmchip-n-npwm)
* [/sys/class/pwm/pwmchip<N>/export](abi-testing.html#abi-sys-class-pwm-pwmchip-n-export)
* [/sys/class/pwm/pwmchip<N>/unexport](abi-testing.html#abi-sys-class-pwm-pwmchip-n-unexport)
* [/sys/class/pwm/pwmchip<N>/pwmX](abi-testing.html#abi-sys-class-pwm-pwmchip-n-pwmx)
* [/sys/class/pwm/pwmchip<N>/pwmX/period](abi-testing.html#abi-sys-class-pwm-pwmchip-n-pwmx-period)
* [/sys/class/pwm/pwmchip<N>/pwmX/duty\_cycle](abi-testing.html#abi-sys-class-pwm-pwmchip-n-pwmx-duty-cycle)
* [/sys/class/pwm/pwmchip<N>/pwmX/polarity](abi-testing.html#abi-sys-class-pwm-pwmchip-n-pwmx-polarity)
* [/sys/class/pwm/pwmchip<N>/pwmX/enable](abi-testing.html#abi-sys-class-pwm-pwmchip-n-pwmx-enable)
* [/sys/class/pwm/pwmchip<N>/pwmX/capture](abi-testing.html#abi-sys-class-pwm-pwmchip-n-pwmx-capture)

## ABI file testing/sysfs-class-rapidio

Has the following ABI:

* [/sys/class/rapidio\_port](abi-testing.html#abi-sys-class-rapidio-port)
* [/sys/class/rapidio\_port/rapidio<N>/sys\_size](abi-testing.html#abi-sys-class-rapidio-port-rapidio-n-sys-size)
* [/sys/class/rapidio\_port/rapidio<N>/port\_destid](abi-testing.html#abi-sys-class-rapidio-port-rapidio-n-port-destid)

## ABI file testing/sysfs-class-rc

Has the following ABI:

* [/sys/class/rc/](abi-testing.html#abi-sys-class-rc)
* [/sys/class/rc/rc<N>/](abi-testing.html#abi-sys-class-rc-rc-n)
* [/sys/class/rc/rc<N>/protocols](abi-testing.html#abi-sys-class-rc-rc-n-protocols)
* [/sys/class/rc/rc<N>/filter](abi-testing.html#abi-sys-class-rc-rc-n-filter)
* [/sys/class/rc/rc<N>/filter\_mask](abi-testing.html#abi-sys-class-rc-rc-n-filter-mask)
* [/sys/class/rc/rc<N>/wakeup\_protocols](abi-testing.html#abi-sys-class-rc-rc-n-wakeup-protocols)
* [/sys/class/rc/rc<N>/wakeup\_filter](abi-testing.html#abi-sys-class-rc-rc-n-wakeup-filter)
* [/sys/class/rc/rc<N>/wakeup\_filter\_mask](abi-testing.html#abi-sys-class-rc-rc-n-wakeup-filter-mask)

## ABI file testing/sysfs-class-rc-nuvoton

Has the following ABI:

* [/sys/class/rc/rc<N>/wakeup\_data](abi-testing.html#abi-sys-class-rc-rc-n-wakeup-data)

## ABI file testing/sysfs-class-reboot-mode-reboot\_modes

Has the following ABI:

* [/sys/class/reboot-mode/<driver>/reboot\_modes](abi-testing.html#abi-sys-class-reboot-mode-driver-reboot-modes)

## ABI file testing/sysfs-class-regulator

Has the following ABI:

* [/sys/class/regulator/.../state](abi-testing.html#abi-sys-class-regulator-state)
* [/sys/class/regulator/.../status](abi-testing.html#abi-sys-class-regulator-status)
* [/sys/class/regulator/.../type](abi-testing.html#abi-sys-class-regulator-type)
* [/sys/class/regulator/.../microvolts](abi-testing.html#abi-sys-class-regulator-microvolts)
* [/sys/class/regulator/.../microamps](abi-testing.html#abi-sys-class-regulator-microamps)
* [/sys/class/regulator/.../opmode](abi-testing.html#abi-sys-class-regulator-opmode)
* [/sys/class/regulator/.../min\_microvolts](abi-testing.html#abi-sys-class-regulator-min-microvolts)
* [/sys/class/regulator/.../max\_microvolts](abi-testing.html#abi-sys-class-regulator-max-microvolts)
* [/sys/class/regulator/.../min\_microamps](abi-testing.html#abi-sys-class-regulator-min-microamps)
* [/sys/class/regulator/.../max\_microamps](abi-testing.html#abi-sys-class-regulator-max-microamps)
* [/sys/class/regulator/.../name](abi-testing.html#abi-sys-class-regulator-name)
* [/sys/class/regulator/.../num\_users](abi-testing.html#abi-sys-class-regulator-num-users)
* [/sys/class/regulator/.../requested\_microamps](abi-testing.html#abi-sys-class-regulator-requested-microamps)
* [/sys/class/regulator/.../parent](abi-testing.html#abi-sys-class-regulator-parent)
* [/sys/class/regulator/.../suspend\_mem\_microvolts](abi-testing.html#abi-sys-class-regulator-suspend-mem-microvolts)
* [/sys/class/regulator/.../suspend\_disk\_microvolts](abi-testing.html#abi-sys-class-regulator-suspend-disk-microvolts)
* [/sys/class/regulator/.../suspend\_standby\_microvolts](abi-testing.html#abi-sys-class-regulator-suspend-standby-microvolts)
* [/sys/class/regulator/.../suspend\_mem\_mode](abi-testing.html#abi-sys-class-regulator-suspend-mem-mode)
* [/sys/class/regulator/.../suspend\_disk\_mode](abi-testing.html#abi-sys-class-regulator-suspend-disk-mode)
* [/sys/class/regulator/.../suspend\_standby\_mode](abi-testing.html#abi-sys-class-regulator-suspend-standby-mode)
* [/sys/class/regulator/.../suspend\_mem\_state](abi-testing.html#abi-sys-class-regulator-suspend-mem-state)
* [/sys/class/regulator/.../suspend\_disk\_state](abi-testing.html#abi-sys-class-regulator-suspend-disk-state)
* [/sys/class/regulator/.../suspend\_standby\_state](abi-testing.html#abi-sys-class-regulator-suspend-standby-state)
* [/sys/class/regulator/.../bypass](abi-testing.html#abi-sys-class-regulator-bypass)
* [/sys/class/regulator/.../under\_voltage](abi-testing.html#abi-sys-class-regulator-under-voltage)
* [/sys/class/regulator/.../over\_current](abi-testing.html#abi-sys-class-regulator-over-current)
* [/sys/class/regulator/.../regulation\_out](abi-testing.html#abi-sys-class-regulator-regulation-out)
* [/sys/class/regulator/.../fail](abi-testing.html#abi-sys-class-regulator-fail)
* [/sys/class/regulator/.../over\_temp](abi-testing.html#abi-sys-class-regulator-over-temp)
* [/sys/class/regulator/.../under\_voltage\_warn](abi-testing.html#abi-sys-class-regulator-under-voltage-warn)
* [/sys/class/regulator/.../over\_current\_warn](abi-testing.html#abi-sys-class-regulator-over-current-warn)
* [/sys/class/regulator/.../over\_voltage\_warn](abi-testing.html#abi-sys-class-regulator-over-voltage-warn)
* [/sys/class/regulator/.../over\_temp\_warn](abi-testing.html#abi-sys-class-regulator-over-temp-warn)

## ABI file testing/sysfs-class-remoteproc

Has the following ABI:

* [/sys/class/remoteproc/.../firmware](abi-testing.html#abi-sys-class-remoteproc-firmware)
* [/sys/class/remoteproc/.../state](abi-testing.html#abi-sys-class-remoteproc-state)
* [/sys/class/remoteproc/.../name](abi-testing.html#abi-sys-class-remoteproc-name)
* [/sys/class/remoteproc/.../coredump](abi-testing.html#abi-sys-class-remoteproc-coredump)
* [/sys/class/remoteproc/.../recovery](abi-testing.html#abi-sys-class-remoteproc-recovery)

## ABI file testing/sysfs-class-rnbd-client

Has the following ABI:

* [/sys/class/rnbd-client](abi-testing.html#abi-sys-class-rnbd-client)
* [/sys/class/rnbd-client/ctl/map\_device](abi-testing.html#abi-sys-class-rnbd-client-ctl-map-device)
* [/sys/class/rnbd-client/ctl/devices/](abi-testing.html#abi-sys-class-rnbd-client-ctl-devices)

## ABI file testing/sysfs-class-rnbd-server

Has the following ABI:

* [/sys/class/rnbd-server](abi-testing.html#abi-sys-class-rnbd-server)
* [/sys/class/rnbd-server/ctl/](abi-testing.html#abi-sys-class-rnbd-server-ctl)
* [/sys/class/rnbd-server/ctl/devices/<device\_name>/block\_dev](abi-testing.html#abi-sys-class-rnbd-server-ctl-devices-device-name-block-dev)
* [/sys/class/rnbd-server/ctl/devices/<device\_name>/sessions/](abi-testing.html#abi-sys-class-rnbd-server-ctl-devices-device-name-sessions)
* [/sys/class/rnbd-server/ctl/devices/<device\_name>/sessions/<session-name>/read\_only](abi-testing.html#abi-sys-class-rnbd-server-ctl-devices-device-name-sessions-session-name-read-only)
* [/sys/class/rnbd-server/ctl/devices/<device\_name>/sessions/<session-name>/mapping\_path](abi-testing.html#abi-sys-class-rnbd-server-ctl-devices-device-name-sessions-session-name-mapping-path)
* [/sys/class/rnbd-server/ctl/devices/<device\_name>/sessions/<session-name>/access\_mode](abi-testing.html#abi-sys-class-rnbd-server-ctl-devices-device-name-sessions-session-name-access-mode)
* [/sys/class/rnbd-server/ctl/devices/<device\_name>/sessions/<session-name>/force\_close](abi-testing.html#abi-sys-class-rnbd-server-ctl-devices-device-name-sessions-session-name-force-close)

## ABI file testing/sysfs-class-rtc

Has the following ABI:

* [/sys/class/rtc/](abi-testing.html#abi-sys-class-rtc)
* [/sys/class/rtc/rtcX/](abi-testing.html#abi-sys-class-rtc-rtcx)
* [/sys/class/rtc/rtcX/date](abi-testing.html#abi-sys-class-rtc-rtcx-date)
* [/sys/class/rtc/rtcX/hctosys](abi-testing.html#abi-sys-class-rtc-rtcx-hctosys)
* [/sys/class/rtc/rtcX/max\_user\_freq](abi-testing.html#abi-sys-class-rtc-rtcx-max-user-freq)
* [/sys/class/rtc/rtcX/name](abi-testing.html#abi-sys-class-rtc-rtcx-name)
* [/sys/class/rtc/rtcX/range](abi-testing.html#abi-sys-class-rtc-rtcx-range)
* [/sys/class/rtc/rtcX/since\_epoch](abi-testing.html#abi-sys-class-rtc-rtcx-since-epoch)
* [/sys/class/rtc/rtcX/time](abi-testing.html#abi-sys-class-rtc-rtcx-time)
* [/sys/class/rtc/rtcX/offset](abi-testing.html#abi-sys-class-rtc-rtcx-offset)
* [/sys/class/rtc/rtcX/wakealarm](abi-testing.html#abi-sys-class-rtc-rtcx-wakealarm)

## ABI file testing/sysfs-class-rtc-rtc0-device-rtc\_calibration

Has the following ABI:

* [/sys/class/rtc/rtc0/device/rtc\_calibration](abi-testing.html#abi-sys-class-rtc-rtc0-device-rtc-calibration)

## ABI file testing/sysfs-class-rtrs-client

Has the following ABI:

* [/sys/class/rtrs-client](abi-testing.html#abi-sys-class-rtrs-client)
* [/sys/class/rtrs-client/<session-name>/add\_path](abi-testing.html#abi-sys-class-rtrs-client-session-name-add-path)
* [/sys/class/rtrs-client/<session-name>/max\_reconnect\_attempts](abi-testing.html#abi-sys-class-rtrs-client-session-name-max-reconnect-attempts)
* [/sys/class/rtrs-client/<session-name>/mp\_policy](abi-testing.html#abi-sys-class-rtrs-client-session-name-mp-policy)
* [/sys/class/rtrs-client/<session-name>/paths/](abi-testing.html#abi-sys-class-rtrs-client-session-name-paths)
* [/sys/class/rtrs-client/<session-name>/paths/<src@dst>/state](abi-testing.html#abi-sys-class-rtrs-client-session-name-paths-src-dst-state)
* [/sys/class/rtrs-client/<session-name>/paths/<src@dst>/reconnect](abi-testing.html#abi-sys-class-rtrs-client-session-name-paths-src-dst-reconnect)
* [/sys/class/rtrs-client/<session-name>/paths/<src@dst>/disconnect](abi-testing.html#abi-sys-class-rtrs-client-session-name-paths-src-dst-disconnect)
* [/sys/class/rtrs-client/<session-name>/paths/<src@dst>/remove\_path](abi-testing.html#abi-sys-class-rtrs-client-session-name-paths-src-dst-remove-path)
* [/sys/class/rtrs-client/<session-name>/paths/<src@dst>/hca\_name](abi-testing.html#abi-sys-class-rtrs-client-session-name-paths-src-dst-hca-name)
* [/sys/class/rtrs-client/<session-name>/paths/<src@dst>/hca\_port](abi-testing.html#abi-sys-class-rtrs-client-session-name-paths-src-dst-hca-port)
* [/sys/class/rtrs-client/<session-name>/paths/<src@dst>/src\_addr](abi-testing.html#abi-sys-class-rtrs-client-session-name-paths-src-dst-src-addr)
* [/sys/class/rtrs-client/<session-name>/paths/<src@dst>/dst\_addr](abi-testing.html#abi-sys-class-rtrs-client-session-name-paths-src-dst-dst-addr)
* [/sys/class/rtrs-client/<session-name>/paths/<src@dst>/cur\_latency](abi-testing.html#abi-sys-class-rtrs-client-session-name-paths-src-dst-cur-latency)
* [/sys/class/rtrs-client/<session-name>/paths/<src@dst>/stats/reset\_all](abi-testing.html#abi-sys-class-rtrs-client-session-name-paths-src-dst-stats-reset-all)
* [/sys/class/rtrs-client/<session-name>/paths/<src@dst>/stats/cpu\_migration](abi-testing.html#abi-sys-class-rtrs-client-session-name-paths-src-dst-stats-cpu-migration)
* [/sys/class/rtrs-client/<session-name>/paths/<src@dst>/stats/reconnects](abi-testing.html#abi-sys-class-rtrs-client-session-name-paths-src-dst-stats-reconnects)
* [/sys/class/rtrs-client/<session-name>/paths/<src@dst>/stats/rdma](abi-testing.html#abi-sys-class-rtrs-client-session-name-paths-src-dst-stats-rdma)

## ABI file testing/sysfs-class-rtrs-server

Has the following ABI:

* [/sys/class/rtrs-server](abi-testing.html#abi-sys-class-rtrs-server)
* [/sys/class/rtrs-server/<session-name>/paths/](abi-testing.html#abi-sys-class-rtrs-server-session-name-paths)
* [/sys/class/rtrs-server/<session-name>/paths/<src@dst>/disconnect](abi-testing.html#abi-sys-class-rtrs-server-session-name-paths-src-dst-disconnect)
* [/sys/class/rtrs-server/<session-name>/paths/<src@dst>/hca\_name](abi-testing.html#abi-sys-class-rtrs-server-session-name-paths-src-dst-hca-name)
* [/sys/class/rtrs-server/<session-name>/paths/<src@dst>/hca\_port](abi-testing.html#abi-sys-class-rtrs-server-session-name-paths-src-dst-hca-port)
* [/sys/class/rtrs-server/<session-name>/paths/<src@dst>/src\_addr](abi-testing.html#abi-sys-class-rtrs-server-session-name-paths-src-dst-src-addr)
* [/sys/class/rtrs-server/<session-name>/paths/<src@dst>/dst\_addr](abi-testing.html#abi-sys-class-rtrs-server-session-name-paths-src-dst-dst-addr)
* [/sys/class/rtrs-server/<session-name>/paths/<src@dst>/stats/rdma](abi-testing.html#abi-sys-class-rtrs-server-session-name-paths-src-dst-stats-rdma)

## ABI file testing/sysfs-class-scsi\_host

Has the following ABI:

* [/sys/class/scsi\_host/hostX/isci\_id](abi-testing.html#abi-sys-class-scsi-host-hostx-isci-id)
* [/sys/class/scsi\_host/hostX/acciopath\_status](abi-testing.html#abi-sys-class-scsi-host-hostx-acciopath-status)
* [/sys/class/scsi\_host/hostX/link\_power\_management\_policy](abi-testing.html#abi-sys-class-scsi-host-hostx-link-power-management-policy)
* [/sys/class/scsi\_host/hostX/em\_message](abi-testing.html#abi-sys-class-scsi-host-hostx-em-message)
* [/sys/class/scsi\_host/hostX/em\_message\_type](abi-testing.html#abi-sys-class-scsi-host-hostx-em-message)
* [/sys/class/scsi\_host/hostX/ahci\_port\_cmd](abi-testing.html#abi-sys-class-scsi-host-hostx-ahci-port-cmd)
* [/sys/class/scsi\_host/hostX/ahci\_host\_caps](abi-testing.html#abi-sys-class-scsi-host-hostx-ahci-port-cmd)
* [/sys/class/scsi\_host/hostX/ahci\_host\_cap2](abi-testing.html#abi-sys-class-scsi-host-hostx-ahci-port-cmd)
* [/sys/class/scsi\_host/hostX/ahci\_host\_version](abi-testing.html#abi-sys-class-scsi-host-hostx-ahci-host-version)
* [/sys/class/scsi\_host/hostX/em\_buffer](abi-testing.html#abi-sys-class-scsi-host-hostx-em-buffer)
* [/sys/class/scsi\_host/hostX/em\_message\_supported](abi-testing.html#abi-sys-class-scsi-host-hostx-em-message-supported)

## ABI file testing/sysfs-class-scsi\_tape

Has the following ABI:

* [/sys/class/scsi\_tape/\*/stats/in\_flight](abi-testing.html#abi-sys-class-scsi-tape-stats-in-flight)
* [/sys/class/scsi\_tape/\*/stats/io\_ns](abi-testing.html#abi-sys-class-scsi-tape-stats-io-ns)
* [/sys/class/scsi\_tape/\*/stats/other\_cnt](abi-testing.html#abi-sys-class-scsi-tape-stats-other-cnt)
* [/sys/class/scsi\_tape/\*/stats/read\_byte\_cnt](abi-testing.html#abi-sys-class-scsi-tape-stats-read-byte-cnt)
* [/sys/class/scsi\_tape/\*/stats/read\_cnt](abi-testing.html#abi-sys-class-scsi-tape-stats-read-cnt)
* [/sys/class/scsi\_tape/\*/stats/read\_ns](abi-testing.html#abi-sys-class-scsi-tape-stats-read-ns)
* [/sys/class/scsi\_tape/\*/stats/write\_byte\_cnt](abi-testing.html#abi-sys-class-scsi-tape-stats-write-byte-cnt)
* [/sys/class/scsi\_tape/\*/stats/write\_cnt](abi-testing.html#abi-sys-class-scsi-tape-stats-write-cnt)
* [/sys/class/scsi\_tape/\*/stats/write\_ms](abi-testing.html#abi-sys-class-scsi-tape-stats-write-ms)
* [/sys/class/scsi\_tape/\*/stats/resid\_cnt](abi-testing.html#abi-sys-class-scsi-tape-stats-resid-cnt)

## ABI file testing/sysfs-class-spi-eeprom

Has the following ABI:

* [/sys/class/spi\_master/spi<bus>/spi<bus>.<dev>/fram](abi-testing.html#abi-sys-class-spi-master-spi-bus-spi-bus-dev-fram)
* [/sys/class/spi\_master/spi<bus>/spi<bus>.<dev>/sernum](abi-testing.html#abi-sys-class-spi-master-spi-bus-spi-bus-dev-sernum)
* [/sys/class/spi\_master/spi<bus>/spi<bus>.<dev>/jedec\_id](abi-testing.html#abi-sys-class-spi-master-spi-bus-spi-bus-dev-jedec-id)

## ABI file testing/sysfs-class-stm

Has the following ABI:

* [/sys/class/stm/<stm>/masters](abi-testing.html#abi-sys-class-stm-stm-masters)
* [/sys/class/stm/<stm>/channels](abi-testing.html#abi-sys-class-stm-stm-channels)
* [/sys/class/stm/<stm>/hw\_override](abi-testing.html#abi-sys-class-stm-stm-hw-override)

## ABI file testing/sysfs-class-stm\_source

Has the following ABI:

* [/sys/class/stm\_source/<stm\_source>/stm\_source\_link](abi-testing.html#abi-sys-class-stm-source-stm-source-stm-source-link)

## ABI file testing/sysfs-class-switchtec

switchtec - Microsemi Switchtec PCI Switch Management Endpoint

For details on this subsystem look at [Linux Switchtec Support](../driver-api/switchtec.html).

Has the following ABI:

* [/sys/class/switchtec](abi-testing.html#abi-sys-class-switchtec)
* [/sys/class/switchtec/switchtec[0-9]+/component\_id](abi-testing.html#abi-sys-class-switchtec-switchtec-0-9-component-id)
* [/sys/class/switchtec/switchtec[0-9]+/component\_revision](abi-testing.html#abi-sys-class-switchtec-switchtec-0-9-component-revision)
* [/sys/class/switchtec/switchtec[0-9]+/component\_vendor](abi-testing.html#abi-sys-class-switchtec-switchtec-0-9-component-vendor)
* [/sys/class/switchtec/switchtec[0-9]+/device\_version](abi-testing.html#abi-sys-class-switchtec-switchtec-0-9-device-version)
* [/sys/class/switchtec/switchtec[0-9]+/fw\_version](abi-testing.html#abi-sys-class-switchtec-switchtec-0-9-fw-version)
* [/sys/class/switchtec/switchtec[0-9]+/partition](abi-testing.html#abi-sys-class-switchtec-switchtec-0-9-partition)
* [/sys/class/switchtec/switchtec[0-9]+/partition\_count](abi-testing.html#abi-sys-class-switchtec-switchtec-0-9-partition-count)
* [/sys/class/switchtec/switchtec[0-9]+/product\_id](abi-testing.html#abi-sys-class-switchtec-switchtec-0-9-product-id)
* [/sys/class/switchtec/switchtec[0-9]+/product\_revision](abi-testing.html#abi-sys-class-switchtec-switchtec-0-9-product-revision)
* [/sys/class/switchtec/switchtec[0-9]+/product\_vendor](abi-testing.html#abi-sys-class-switchtec-switchtec-0-9-product-vendor)

## ABI file testing/sysfs-class-tee

Has the following ABI:

* [/sys/class/tee/tee{,priv}X/rpmb\_routing\_model](abi-testing.html#abi-sys-class-tee-tee-priv-x-rpmb-routing-model)
* [/sys/class/tee/tee{,priv}X/revision](abi-testing.html#abi-sys-class-tee-tee-priv-x-revision)

## ABI file testing/sysfs-class-thermal

Has the following ABI:

* [/sys/class/thermal/thermal\_zoneX/type](abi-testing.html#abi-sys-class-thermal-thermal-zonex-type)
* [/sys/class/thermal/thermal\_zoneX/temp](abi-testing.html#abi-sys-class-thermal-thermal-zonex-temp)
* [/sys/class/thermal/thermal\_zoneX/mode](abi-testing.html#abi-sys-class-thermal-thermal-zonex-mode)
* [/sys/class/thermal/thermal\_zoneX/policy](abi-testing.html#abi-sys-class-thermal-thermal-zonex-policy)
* [/sys/class/thermal/thermal\_zoneX/available\_policies](abi-testing.html#abi-sys-class-thermal-thermal-zonex-available-policies)
* [/sys/class/thermal/thermal\_zoneX/trip\_point\_Y\_temp](abi-testing.html#abi-sys-class-thermal-thermal-zonex-trip-point-y-temp)
* [/sys/class/thermal/thermal\_zoneX/trip\_point\_Y\_type](abi-testing.html#abi-sys-class-thermal-thermal-zonex-trip-point-y-type)
* [/sys/class/thermal/thermal\_zoneX/trip\_point\_Y\_hyst](abi-testing.html#abi-sys-class-thermal-thermal-zonex-trip-point-y-hyst)
* [/sys/class/thermal/thermal\_zoneX/cdevY](abi-testing.html#abi-sys-class-thermal-thermal-zonex-cdevy)
* [/sys/class/thermal/thermal\_zoneX/cdevY\_trip\_point](abi-testing.html#abi-sys-class-thermal-thermal-zonex-cdevy-trip-point)
* [/sys/class/thermal/thermal\_zoneX/cdevY\_weight](abi-testing.html#abi-sys-class-thermal-thermal-zonex-cdevy-weight)
* [/sys/class/thermal/thermal\_zoneX/emul\_temp](abi-testing.html#abi-sys-class-thermal-thermal-zonex-emul-temp)
* [/sys/class/thermal/thermal\_zoneX/k\_d](abi-testing.html#abi-sys-class-thermal-thermal-zonex-k-d)
* [/sys/class/thermal/thermal\_zoneX/k\_i](abi-testing.html#abi-sys-class-thermal-thermal-zonex-k-i)
* [/sys/class/thermal/thermal\_zoneX/k\_po](abi-testing.html#abi-sys-class-thermal-thermal-zonex-k-po)
* [/sys/class/thermal/thermal\_zoneX/k\_pu](abi-testing.html#abi-sys-class-thermal-thermal-zonex-k-pu)
* [/sys/class/thermal/thermal\_zoneX/integral\_cutoff](abi-testing.html#abi-sys-class-thermal-thermal-zonex-integral-cutoff)
* [/sys/class/thermal/thermal\_zoneX/slope](abi-testing.html#abi-sys-class-thermal-thermal-zonex-slope)
* [/sys/class/thermal/thermal\_zoneX/offset](abi-testing.html#abi-sys-class-thermal-thermal-zonex-offset)
* [/sys/class/thermal/thermal\_zoneX/sustainable\_power](abi-testing.html#abi-sys-class-thermal-thermal-zonex-sustainable-power)
* [/sys/class/thermal/cooling\_deviceX/type](abi-testing.html#abi-sys-class-thermal-cooling-devicex-type)
* [/sys/class/thermal/cooling\_deviceX/max\_state](abi-testing.html#abi-sys-class-thermal-cooling-devicex-max-state)
* [/sys/class/thermal/cooling\_deviceX/cur\_state](abi-testing.html#abi-sys-class-thermal-cooling-devicex-cur-state)
* [/sys/class/thermal/cooling\_deviceX/stats/reset](abi-testing.html#abi-sys-class-thermal-cooling-devicex-stats-reset)
* [/sys/class/thermal/cooling\_deviceX/stats/time\_in\_state\_ms:](abi-testing.html#abi-sys-class-thermal-cooling-devicex-stats-time-in-state-ms)
* [/sys/class/thermal/cooling\_deviceX/stats/total\_trans](abi-testing.html#abi-sys-class-thermal-cooling-devicex-stats-total-trans)
* [/sys/class/thermal/cooling\_deviceX/stats/trans\_table](abi-testing.html#abi-sys-class-thermal-cooling-devicex-stats-trans-table)

## ABI file testing/sysfs-class-tsm

Has the following ABI:

* [/sys/class/tsm/tsmN](abi-testing.html#abi-sys-class-tsm-tsmn)

## ABI file testing/sysfs-class-typec

USB Type-C port devices (eg. /sys/class/typec/port0/)

Has the following ABI:

* [/sys/class/typec/<port>/data\_role](abi-testing.html#abi-sys-class-typec-port-data-role)
* [/sys/class/typec/<port>/power\_role](abi-testing.html#abi-sys-class-typec-port-power-role)
* [/sys/class/typec/<port>/port\_type](abi-testing.html#abi-sys-class-typec-port-port-type)
* [/sys/class/typec/<port>/vconn\_source](abi-testing.html#abi-sys-class-typec-port-vconn-source)
* [/sys/class/typec/<port>/power\_operation\_mode](abi-testing.html#abi-sys-class-typec-port-power-operation-mode)
* [/sys/class/typec/<port>/preferred\_role](abi-testing.html#abi-sys-class-typec-port-preferred-role)
* [/sys/class/typec/<port>/supported\_accessory\_modes](abi-testing.html#abi-sys-class-typec-port-supported-accessory-modes)
* [/sys/class/typec/<port>/usb\_power\_delivery\_revision](abi-testing.html#abi-sys-class-typec-port-usb-power-delivery-revision)
* [/sys/class/typec/<port>-{partner|cable}/usb\_power\_delivery\_revision](abi-testing.html#abi-sys-class-typec-port-partner-cable-usb-power-delivery-revision)
* [/sys/class/typec/<port>/usb\_typec\_revision](abi-testing.html#abi-sys-class-typec-port-usb-typec-revision)
* [/sys/class/typec/<port>/orientation](abi-testing.html#abi-sys-class-typec-port-orientation)
* [/sys/class/typec/<port>/select\_usb\_power\_delivery](abi-testing.html#abi-sys-class-typec-port-select-usb-power-delivery)
* [/sys/class/typec/<port>/usb\_capability](abi-testing.html#abi-sys-class-typec-port-usb-capability)
* [/sys/class/typec/<port>/<alt-mode>/priority](abi-testing.html#abi-sys-class-typec-port-alt-mode-priority)
* [/sys/class/typec/<port>-partner/accessory\_mode](abi-testing.html#abi-sys-class-typec-port-partner-accessory-mode)
* [/sys/class/typec/<port>-partner/supports\_usb\_power\_delivery](abi-testing.html#abi-sys-class-typec-port-partner-supports-usb-power-delivery)
* [/sys/class/typec/<port>-partner/number\_of\_alternate\_modes](abi-testing.html#abi-sys-class-typec-port-partner-number-of-alternate-modes)
* [/sys/class/typec/<port>-partner/type](abi-testing.html#abi-sys-class-typec-port-partner-type)
* [/sys/class/typec/<port>-partner/identity/](abi-testing.html#abi-sys-class-typec-port-partner-identity)
* [/sys/class/typec/<port>-partner/usb\_mode](abi-testing.html#abi-sys-class-typec-port-partner-usb-mode)
* [/sys/class/typec/<port>-cable/type](abi-testing.html#abi-sys-class-typec-port-cable-type)
* [/sys/class/typec/<port>-cable/plug\_type](abi-testing.html#abi-sys-class-typec-port-cable-plug-type)
* [/sys/class/typec/<port>-<plug>/number\_of\_alternate\_modes](abi-testing.html#abi-sys-class-typec-port-plug-number-of-alternate-modes)
* [/sys/class/typec/<port>-{partner|cable}/identity/](abi-testing.html#abi-sys-class-typec-port-partner-cable-identity)
* [/sys/class/typec/<port>-{partner|cable}/identity/id\_header](abi-testing.html#abi-sys-class-typec-port-partner-cable-identity-id-header)
* [/sys/class/typec/<port>-{partner|cable}/identity/cert\_stat](abi-testing.html#abi-sys-class-typec-port-partner-cable-identity-cert-stat)
* [/sys/class/typec/<port>-{partner|cable}/identity/product](abi-testing.html#abi-sys-class-typec-port-partner-cable-identity-product)
* [/sys/class/typec/<port>-{partner|cable}/identity/product\_type\_vdo1](abi-testing.html#abi-sys-class-typec-port-partner-cable-identity-product-type-vdo1)
* [/sys/class/typec/<port>-{partner|cable}/identity/product\_type\_vdo2](abi-testing.html#abi-sys-class-typec-port-partner-cable-identity-product-type-vdo2)
* [/sys/class/typec/<port>-{partner|cable}/identity/product\_type\_vdo3](abi-testing.html#abi-sys-class-typec-port-partner-cable-identity-product-type-vdo3)
* [/sys/class/typec/<port>/<alt mode>/supported\_roles](abi-testing.html#abi-sys-class-typec-port-alt-mode-supported-roles)

## ABI file testing/sysfs-class-usb\_power\_delivery

Has the following ABI:

* [/sys/class/usb\_power\_delivery](abi-testing.html#abi-sys-class-usb-power-delivery)
* [/sys/class/usb\_power\_delivery/.../revision](abi-testing.html#abi-sys-class-usb-power-delivery-revision)
* [/sys/class/usb\_power\_delivery/.../version](abi-testing.html#abi-sys-class-usb-power-delivery-version)
* [/sys/class/usb\_power\_delivery/.../source-capabilities](abi-testing.html#abi-sys-class-usb-power-delivery-source-capabilities)
* [/sys/class/usb\_power\_delivery/.../sink-capabilities](abi-testing.html#abi-sys-class-usb-power-delivery-sink-capabilities)
* [/sys/class/usb\_power\_delivery/.../<capability>/<position>:fixed\_supply](abi-testing.html#abi-sys-class-usb-power-delivery-capability-position-fixed-supply)
* [/sys/class/usb\_power\_delivery/.../<capability>/1:fixed\_supply/dual\_role\_power](abi-testing.html#abi-sys-class-usb-power-delivery-capability-1-fixed-supply-dual-role-power)
* [/sys/class/usb\_power\_delivery/.../source-capabilities/1:fixed\_supply/usb\_suspend\_supported](abi-testing.html#abi-sys-class-usb-power-delivery-source-capabilities-1-fixed-supply-usb-suspend-supported)
* [/sys/class/usb\_power\_delivery/.../sink-capabilities/1:fixed\_supply/higher\_capability](abi-testing.html#abi-sys-class-usb-power-delivery-sink-capabilities-1-fixed-supply-higher-capability)
* [/sys/class/usb\_power\_delivery/.../<capability>/1:fixed\_supply/unconstrained\_power](abi-testing.html#abi-sys-class-usb-power-delivery-capability-1-fixed-supply-unconstrained-power)
* [/sys/class/usb\_power\_delivery/.../<capability>/1:fixed\_supply/usb\_communication\_capable](abi-testing.html#abi-sys-class-usb-power-delivery-capability-1-fixed-supply-usb-communication-capable)
* [/sys/class/usb\_power\_delivery/.../<capability>/1:fixed\_supply/dual\_role\_data](abi-testing.html#abi-sys-class-usb-power-delivery-capability-1-fixed-supply-dual-role-data)
* [/sys/class/usb\_power\_delivery/.../<capability>/1:fixed\_supply/unchunked\_extended\_messages\_supported](abi-testing.html#abi-sys-class-usb-power-delivery-capability-1-fixed-supply-unchunked-extended-messages-supported)
* [/sys/class/usb\_power\_delivery/.../<capability>/<position>:fixed\_supply/voltage](abi-testing.html#abi-sys-class-usb-power-delivery-capability-position-fixed-supply-voltage)
* [/sys/class/usb\_power\_delivery/.../source-capabilities/<position>:fixed\_supply/peak\_current](abi-testing.html#abi-sys-class-usb-power-delivery-source-capabilities-position-fixed-supply-peak-current)
* [/sys/class/usb\_power\_delivery/.../source-capabilities/<position>:fixed\_supply/maximum\_current](abi-testing.html#abi-sys-class-usb-power-delivery-source-capabilities-position-fixed-supply-maximum-current)
* [/sys/class/usb\_power\_delivery/.../sink-capabilities/<position>:fixed\_supply/operational\_current](abi-testing.html#abi-sys-class-usb-power-delivery-sink-capabilities-position-fixed-supply-operational-current)
* [/sys/class/usb\_power\_delivery/.../sink-capabilities/<position>:fixed\_supply/fast\_role\_swap\_current](abi-testing.html#abi-sys-class-usb-power-delivery-sink-capabilities-position-fixed-supply-fast-role-swap-current)
* [/sys/class/usb\_power\_delivery/.../<capability>/<position>:variable\_supply](abi-testing.html#abi-sys-class-usb-power-delivery-capability-position-variable-supply)
* [/sys/class/usb\_power\_delivery/.../<capability>/<position>:variable\_supply/maximum\_voltage](abi-testing.html#abi-sys-class-usb-power-delivery-capability-position-variable-supply-maximum-voltage)
* [/sys/class/usb\_power\_delivery/.../<capability>/<position>:variable\_supply/minimum\_voltage](abi-testing.html#abi-sys-class-usb-power-delivery-capability-position-variable-supply-minimum-voltage)
* [/sys/class/usb\_power\_delivery/.../source-capabilities/<position>:variable\_supply/maximum\_current](abi-testing.html#abi-sys-class-usb-power-delivery-source-capabilities-position-variable-supply-maximum-current)
* [/sys/class/usb\_power\_delivery/.../sink-capabilities/<position>:variable\_supply/operational\_current](abi-testing.html#abi-sys-class-usb-power-delivery-sink-capabilities-position-variable-supply-operational-current)
* [/sys/class/usb\_power\_delivery/.../<capability>/<position>:battery](abi-testing.html#abi-sys-class-usb-power-delivery-capability-position-battery)
* [/sys/class/usb\_power\_delivery/.../<capability>/<position>:battery/maximum\_voltage](abi-testing.html#abi-sys-class-usb-power-delivery-capability-position-battery-maximum-voltage)
* [/sys/class/usb\_power\_delivery/.../<capability>/<position>:battery/minimum\_voltage](abi-testing.html#abi-sys-class-usb-power-delivery-capability-position-battery-minimum-voltage)
* [/sys/class/usb\_power\_delivery/.../source-capabilities/<position>:battery/maximum\_power](abi-testing.html#abi-sys-class-usb-power-delivery-source-capabilities-position-battery-maximum-power)
* [/sys/class/usb\_power\_delivery/.../sink-capabilities/<position>:battery/operational\_power](abi-testing.html#abi-sys-class-usb-power-delivery-sink-capabilities-position-battery-operational-power)
* [/sys/class/usb\_power\_delivery/.../<capability>/<position>:programmable\_supply](abi-testing.html#abi-sys-class-usb-power-delivery-capability-position-programmable-supply)
* [/sys/class/usb\_power\_delivery/.../<capability>/<position>:programmable\_supply/maximum\_voltage](abi-testing.html#abi-sys-class-usb-power-delivery-capability-position-programmable-supply-maximum-voltage)
* [/sys/class/usb\_power\_delivery/.../<capability>/<position>:programmable\_supply/minimum\_voltage](abi-testing.html#abi-sys-class-usb-power-delivery-capability-position-programmable-supply-minimum-voltage)
* [/sys/class/usb\_power\_delivery/.../<capability>/<position>:programmable\_supply/maximum\_current](abi-testing.html#abi-sys-class-usb-power-delivery-capability-position-programmable-supply-maximum-current)
* [/sys/class/usb\_power\_delivery/.../source-capabilities/<position>:programmable\_supply/pps\_power\_limited](abi-testing.html#abi-sys-class-usb-power-delivery-source-capabilities-position-programmable-supply-pps-power-limited)
* [/sys/class/usb\_power\_delivery/.../<capability>/<position>:spr\_adjustable\_voltage\_supply](abi-testing.html#abi-sys-class-usb-power-delivery-capability-position-spr-adjustable-voltage-supply)
* [/sys/class/usb\_power\_delivery/.../<capability>/<position>:spr\_adjustable\_voltage\_supply/maximum\_current\_9V\_to\_15V](abi-testing.html#abi-sys-class-usb-power-delivery-capability-position-spr-adjustable-voltage-supply-maximum-current-9v-to-15v)
* [/sys/class/usb\_power\_delivery/.../<capability>/<position>:spr\_adjustable\_voltage\_supply/maximum\_current\_15V\_to\_20V](abi-testing.html#abi-sys-class-usb-power-delivery-capability-position-spr-adjustable-voltage-supply-maximum-current-15v-to-20v)
* [/sys/class/usb\_power\_delivery/.../<capability>/<position>:spr\_adjustable\_voltage\_supply/peak\_current](abi-testing.html#abi-sys-class-usb-power-delivery-capability-position-spr-adjustable-voltage-supply-peak-current)

## ABI file testing/sysfs-class-usb\_role

Has the following ABI:

* [/sys/class/usb\_role/](abi-testing.html#abi-sys-class-usb-role)
* [/sys/class/usb\_role/<switch>/role](abi-testing.html#abi-sys-class-usb-role-switch-role)
* [/sys/class/usb\_role/<switch>/connector](abi-testing.html#abi-sys-class-usb-role-switch-connector)

## ABI file testing/sysfs-class-vduse

Has the following ABI:

* [/sys/class/vduse/](abi-testing.html#abi-sys-class-vduse)
* [/sys/class/vduse/control/](abi-testing.html#abi-sys-class-vduse-control)
* [/sys/class/vduse/<device-name>/](abi-testing.html#abi-sys-class-vduse-device-name)
* [/sys/class/vduse/<device-name>/msg\_timeout](abi-testing.html#abi-sys-class-vduse-device-name-msg-timeout)

## ABI file testing/sysfs-class-wakeup

Has the following ABI:

* [/sys/class/wakeup/](abi-testing.html#abi-sys-class-wakeup)
* [/sys/class/wakeup/.../name](abi-testing.html#abi-sys-class-wakeup-name)
* [/sys/class/wakeup/.../active\_count](abi-testing.html#abi-sys-class-wakeup-active-count)
* [/sys/class/wakeup/.../event\_count](abi-testing.html#abi-sys-class-wakeup-event-count)
* [/sys/class/wakeup/.../wakeup\_count](abi-testing.html#abi-sys-class-wakeup-wakeup-count)
* [/sys/class/wakeup/.../expire\_count](abi-testing.html#abi-sys-class-wakeup-expire-count)
* [/sys/class/wakeup/.../active\_time\_ms](abi-testing.html#abi-sys-class-wakeup-active-time-ms)
* [/sys/class/wakeup/.../total\_time\_ms](abi-testing.html#abi-sys-class-wakeup-total-time-ms)
* [/sys/class/wakeup/.../max\_time\_ms](abi-testing.html#abi-sys-class-wakeup-max-time-ms)
* [/sys/class/wakeup/.../last\_change\_ms](abi-testing.html#abi-sys-class-wakeup-last-change-ms)
* [/sys/class/wakeup/.../prevent\_suspend\_time\_ms](abi-testing.html#abi-sys-class-wakeup-prevent-suspend-time-ms)

## ABI file testing/sysfs-class-watchdog

Has the following ABI:

* [/sys/class/watchdog/watchdogn/bootstatus](abi-testing.html#abi-sys-class-watchdog-watchdogn-bootstatus)
* [/sys/class/watchdog/watchdogn/options](abi-testing.html#abi-sys-class-watchdog-watchdogn-options)
* [/sys/class/watchdog/watchdogn/fw\_version](abi-testing.html#abi-sys-class-watchdog-watchdogn-fw-version)
* [/sys/class/watchdog/watchdogn/identity](abi-testing.html#abi-sys-class-watchdog-watchdogn-identity)
* [/sys/class/watchdog/watchdogn/nowayout](abi-testing.html#abi-sys-class-watchdog-watchdogn-nowayout)
* [/sys/class/watchdog/watchdogn/state](abi-testing.html#abi-sys-class-watchdog-watchdogn-state)
* [/sys/class/watchdog/watchdogn/status](abi-testing.html#abi-sys-class-watchdog-watchdogn-status)
* [/sys/class/watchdog/watchdogn/timeleft](abi-testing.html#abi-sys-class-watchdog-watchdogn-timeleft)
* [/sys/class/watchdog/watchdogn/timeout](abi-testing.html#abi-sys-class-watchdog-watchdogn-timeout)
* [/sys/class/watchdog/watchdogn/pretimeout](abi-testing.html#abi-sys-class-watchdog-watchdogn-pretimeout)
* [/sys/class/watchdog/watchdogn/pretimeout\_available\_governors](abi-testing.html#abi-sys-class-watchdog-watchdogn-pretimeout-available-governors)
* [/sys/class/watchdog/watchdogn/pretimeout\_governor](abi-testing.html#abi-sys-class-watchdog-watchdogn-pretimeout-governor)
* [/sys/class/watchdog/watchdog1/access\_cs0](abi-testing.html#abi-sys-class-watchdog-watchdog1-access-cs0)

## ABI file testing/sysfs-class-zram

Has the following ABI:

* [/sys/class/zram-control/](abi-testing.html#abi-sys-class-zram-control)
* [/sys/class/zram-control/hot\_add](abi-testing.html#abi-sys-class-zram-control-hot-add)
* [/sys/class/zram-control/hot\_remove](abi-testing.html#abi-sys-class-zram-control-hot-remove)

## ABI file testing/sysfs-dev

Has the following ABI:

* [/sys/dev](abi-testing.html#abi-sys-dev)

## ABI file testing/sysfs-devices

Has the following ABI:

* [/sys/devices](abi-testing.html#abi-sys-devices)

## ABI file testing/sysfs-devices-consumer

Has the following ABI:

* [/sys/devices/.../consumer:<consumer>](abi-testing.html#abi-sys-devices-consumer-consumer)

## ABI file testing/sysfs-devices-coredump

Has the following ABI:

* [/sys/devices/.../coredump](abi-testing.html#abi-sys-devices-coredump)

## ABI file testing/sysfs-devices-edac

Has the following ABI:

* [/sys/devices/system/edac/mc/mc\*/reset\_counters](abi-testing.html#abi-sys-devices-system-edac-mc-mc-reset-counters)
* [/sys/devices/system/edac/mc/mc\*/seconds\_since\_reset](abi-testing.html#abi-sys-devices-system-edac-mc-mc-seconds-since-reset)
* [/sys/devices/system/edac/mc/mc\*/mc\_name](abi-testing.html#abi-sys-devices-system-edac-mc-mc-mc-name)
* [/sys/devices/system/edac/mc/mc\*/size\_mb](abi-testing.html#abi-sys-devices-system-edac-mc-mc-size-mb)
* [/sys/devices/system/edac/mc/mc\*/ue\_count](abi-testing.html#abi-sys-devices-system-edac-mc-mc-ue-count)
* [/sys/devices/system/edac/mc/mc\*/ue\_noinfo\_count](abi-testing.html#abi-sys-devices-system-edac-mc-mc-ue-noinfo-count)
* [/sys/devices/system/edac/mc/mc\*/ce\_count](abi-testing.html#abi-sys-devices-system-edac-mc-mc-ce-count)
* [/sys/devices/system/edac/mc/mc\*/ce\_noinfo\_count](abi-testing.html#abi-sys-devices-system-edac-mc-mc-ce-noinfo-count)
* [/sys/devices/system/edac/mc/mc\*/sdram\_scrub\_rate](abi-testing.html#abi-sys-devices-system-edac-mc-mc-sdram-scrub-rate)
* [/sys/devices/system/edac/mc/mc\*/max\_location](abi-testing.html#abi-sys-devices-system-edac-mc-mc-max-location)
* [/sys/devices/system/edac/mc/mc\*/(dimm|rank)\*/size](abi-testing.html#abi-sys-devices-system-edac-mc-mc-dimm-rank-size)
* [/sys/devices/system/edac/mc/mc\*/(dimm|rank)\*/dimm\_dev\_type](abi-testing.html#abi-sys-devices-system-edac-mc-mc-dimm-rank-dimm-dev-type)
* [/sys/devices/system/edac/mc/mc\*/(dimm|rank)\*/dimm\_edac\_mode](abi-testing.html#abi-sys-devices-system-edac-mc-mc-dimm-rank-dimm-edac-mode)
* [/sys/devices/system/edac/mc/mc\*/(dimm|rank)\*/dimm\_label](abi-testing.html#abi-sys-devices-system-edac-mc-mc-dimm-rank-dimm-label)
* [/sys/devices/system/edac/mc/mc\*/(dimm|rank)\*/dimm\_location](abi-testing.html#abi-sys-devices-system-edac-mc-mc-dimm-rank-dimm-location)
* [/sys/devices/system/edac/mc/mc\*/(dimm|rank)\*/dimm\_mem\_type](abi-testing.html#abi-sys-devices-system-edac-mc-mc-dimm-rank-dimm-mem-type)
* [/sys/devices/system/edac/mc/mc\*/(dimm|rank)\*/dimm\_ce\_count](abi-testing.html#abi-sys-devices-system-edac-mc-mc-dimm-rank-dimm-ce-count)
* [/sys/devices/system/edac/mc/mc\*/(dimm|rank)\*/dimm\_ue\_count](abi-testing.html#abi-sys-devices-system-edac-mc-mc-dimm-rank-dimm-ue-count)

## ABI file testing/sysfs-devices-firmware\_node

Has the following ABI:

* [/sys/devices/.../firmware\_node/](abi-testing.html#abi-sys-devices-firmware-node)
* [/sys/devices/.../firmware\_node/description](abi-testing.html#abi-sys-devices-firmware-node-description)

## ABI file testing/sysfs-devices-lpss\_ltr

Has the following ABI:

* [/sys/devices/.../lpss\_ltr/](abi-testing.html#abi-sys-devices-lpss-ltr)
* [/sys/devices/.../lpss\_ltr/ltr\_mode](abi-testing.html#abi-sys-devices-lpss-ltr-ltr-mode)
* [/sys/devices/.../lpss\_ltr/auto\_ltr](abi-testing.html#abi-sys-devices-lpss-ltr-auto-ltr)
* [/sys/devices/.../lpss\_ltr/sw\_ltr](abi-testing.html#abi-sys-devices-lpss-ltr-sw-ltr)

## ABI file testing/sysfs-devices-mapping

Has the following ABI:

* [/sys/devices/uncore\_iio\_x/dieX](abi-testing.html#abi-sys-devices-uncore-iio-x-diex)
* [/sys/devices/uncore\_upi\_x/dieX](abi-testing.html#abi-sys-devices-uncore-upi-x-diex)

## ABI file testing/sysfs-devices-memory

Has the following ABI:

* [/sys/devices/system/memory](abi-testing.html#abi-sys-devices-system-memory)
* [/sys/devices/system/memory/memoryX/removable](abi-testing.html#abi-sys-devices-system-memory-memoryx-removable)
* [/sys/devices/system/memory/memoryX/phys\_device](abi-testing.html#abi-sys-devices-system-memory-memoryx-phys-device)
* [/sys/devices/system/memory/memoryX/phys\_index](abi-testing.html#abi-sys-devices-system-memory-memoryx-phys-index)
* [/sys/devices/system/memory/memoryX/state](abi-testing.html#abi-sys-devices-system-memory-memoryx-state)
* [/sys/devices/system/memory/memoryX/valid\_zones](abi-testing.html#abi-sys-devices-system-memory-memoryx-valid-zones)
* [/sys/devices/system/memoryX/nodeY](abi-testing.html#abi-sys-devices-system-memoryx-nodey)
* [/sys/devices/system/node/nodeX/memoryY](abi-testing.html#abi-sys-devices-system-node-nodex-memoryy)
* [/sys/devices/system/memory/crash\_hotplug](abi-testing.html#abi-sys-devices-system-memory-crash-hotplug)

## ABI file testing/sysfs-devices-mmc

Has the following ABI:

* [/sys/devices/.../mmc\_host/mmcX/mmcX:XXXX/enhanced\_area\_offset](abi-testing.html#abi-sys-devices-mmc-host-mmcx-mmcx-xxxx-enhanced-area-offset)
* [/sys/devices/.../mmc\_host/mmcX/mmcX:XXXX/enhanced\_area\_size](abi-testing.html#abi-sys-devices-mmc-host-mmcx-mmcx-xxxx-enhanced-area-size)

## ABI file testing/sysfs-devices-online

Has the following ABI:

* [/sys/devices/.../online](abi-testing.html#abi-sys-devices-online)

## ABI file testing/sysfs-devices-pci-host-bridge

Has the following ABI:

* [/sys/devices/pciDDDD:BB](abi-testing.html#abi-sys-devices-pcidddd-bb)
* [pciDDDD:BB/firmware\_node](abi-testing.html#abi-pcidddd-bb-firmware-node)
* [pciDDDD:BB/streamH.R.E](abi-testing.html#abi-pcidddd-bb-streamh-r-e)
* [pciDDDD:BB/available\_secure\_streams](abi-testing.html#abi-pcidddd-bb-available-secure-streams)

## ABI file testing/sysfs-devices-physical\_location

Has the following ABI:

* [/sys/devices/.../physical\_location](abi-testing.html#abi-sys-devices-physical-location)
* [/sys/devices/.../physical\_location/panel](abi-testing.html#abi-sys-devices-physical-location-panel)
* [/sys/devices/.../physical\_location/vertical\_position](abi-testing.html#abi-sys-devices-physical-location-vertical-position)
* [/sys/devices/.../physical\_location/horizontal\_position](abi-testing.html#abi-sys-devices-physical-location-horizontal-position)
* [/sys/devices/.../physical\_location/dock](abi-testing.html#abi-sys-devices-physical-location-dock)
* [/sys/devices/.../physical\_location/lid](abi-testing.html#abi-sys-devices-physical-location-lid)

## ABI file testing/sysfs-devices-platform-ACPI-TAD

> ACPI Time and Alarm (TAD) device attributes.

Has the following ABI:

* [/sys/bus/platform/devices/ACPI000E:00/caps](abi-testing.html#abi-sys-bus-platform-devices-acpi000e-00-caps)
* [/sys/bus/platform/devices/ACPI000E:00/ac\_alarm](abi-testing.html#abi-sys-bus-platform-devices-acpi000e-00-ac-alarm)
* [/sys/bus/platform/devices/ACPI000E:00/ac\_policy](abi-testing.html#abi-sys-bus-platform-devices-acpi000e-00-ac-policy)
* [/sys/bus/platform/devices/ACPI000E:00/ac\_status](abi-testing.html#abi-sys-bus-platform-devices-acpi000e-00-ac-status)
* [/sys/bus/platform/devices/ACPI000E:00/dc\_alarm](abi-testing.html#abi-sys-bus-platform-devices-acpi000e-00-dc-alarm)
* [/sys/bus/platform/devices/ACPI000E:00/dc\_policy](abi-testing.html#abi-sys-bus-platform-devices-acpi000e-00-dc-policy)
* [/sys/bus/platform/devices/ACPI000E:00/dc\_status](abi-testing.html#abi-sys-bus-platform-devices-acpi000e-00-dc-status)

## ABI file testing/sysfs-devices-platform-\_UDC\_-gadget

Has the following ABI:

* [/sys/devices/platform/\_UDC\_/gadget/suspended](abi-testing.html#abi-sys-devices-platform-udc-gadget-suspended)
* [/sys/devices/platform/\_UDC\_/gadget/gadget-lunX/nofua](abi-testing.html#abi-sys-devices-platform-udc-gadget-gadget-lunx-nofua)

## ABI file testing/sysfs-devices-platform-docg3

Has the following ABI:

* [/sys/devices/platform/docg3/f[0-3]\_dps[01]\_is\_keylocked](abi-testing.html#abi-sys-devices-platform-docg3-f-0-3-dps-01-is-keylocked)
* [/sys/devices/platform/docg3/f[0-3]\_dps[01]\_protection\_key](abi-testing.html#abi-sys-devices-platform-docg3-f-0-3-dps-01-protection-key)

## ABI file testing/sysfs-devices-platform-dock

Has the following ABI:

* [/sys/devices/platform/dock.<N>/docked](abi-testing.html#abi-sys-devices-platform-dock-n-docked)
* [/sys/devices/platform/dock.<N>/undock](abi-testing.html#abi-sys-devices-platform-dock-n-undock)
* [/sys/devices/platform/dock.<N>/uid](abi-testing.html#abi-sys-devices-platform-dock-n-uid)
* [/sys/devices/platform/dock.<N>/flags](abi-testing.html#abi-sys-devices-platform-dock-n-flags)
* [/sys/devices/platform/dock.<N>/type](abi-testing.html#abi-sys-devices-platform-dock-n-type)

## ABI file testing/sysfs-devices-platform-ipmi

Has the following ABI:

* [/sys/devices/platform/ipmi\_bmc.\*/firmware\_revision](abi-testing.html#abi-sys-devices-platform-ipmi-bmc-firmware-revision)
* [/sys/devices/platform/ipmi\_bmc.\*/aux\_firmware\_revision](abi-testing.html#abi-sys-devices-platform-ipmi-bmc-aux-firmware-revision)
* [/sys/devices/platform/ipmi\_bmc.\*/revision](abi-testing.html#abi-sys-devices-platform-ipmi-bmc-revision)
* [/sys/devices/platform/ipmi\_bmc.\*/provides\_device\_sdrs](abi-testing.html#abi-sys-devices-platform-ipmi-bmc-provides-device-sdrs)
* [/sys/devices/platform/ipmi\_bmc.\*/device\_id](abi-testing.html#abi-sys-devices-platform-ipmi-bmc-device-id)
* [/sys/devices/platform/ipmi\_bmc.\*/additional\_device\_support](abi-testing.html#abi-sys-devices-platform-ipmi-bmc-additional-device-support)
* [/sys/devices/platform/ipmi\_bmc.\*/ipmi\_version](abi-testing.html#abi-sys-devices-platform-ipmi-bmc-ipmi-version)
* [/sys/devices/platform/ipmi\_bmc.\*/manufacturer\_id](abi-testing.html#abi-sys-devices-platform-ipmi-bmc-manufacturer-id)
* [/sys/devices/platform/ipmi\_bmc.\*/product\_id](abi-testing.html#abi-sys-devices-platform-ipmi-bmc-product-id)
* [/sys/devices/platform/ipmi\_bmc.\*/guid](abi-testing.html#abi-sys-devices-platform-ipmi-bmc-guid)
* [/sys/devices/platform/ipmi\_si.\*/type](abi-testing.html#abi-sys-devices-platform-ipmi-si-type)
* [/sys/devices/platform/ipmi\_si.\*/idles](abi-testing.html#abi-sys-devices-platform-ipmi-si-idles)
* [/sys/devices/platform/ipmi\_si.\*/watchdog\_pretimeouts](abi-testing.html#abi-sys-devices-platform-ipmi-si-idles)
* [/sys/devices/platform/ipmi\_si.\*/complete\_transactions](abi-testing.html#abi-sys-devices-platform-ipmi-si-idles)
* [/sys/devices/platform/ipmi\_si.\*/events](abi-testing.html#abi-sys-devices-platform-ipmi-si-idles)
* [/sys/devices/platform/ipmi\_si.\*/interrupts](abi-testing.html#abi-sys-devices-platform-ipmi-si-idles)
* [/sys/devices/platform/ipmi\_si.\*/hosed\_count](abi-testing.html#abi-sys-devices-platform-ipmi-si-idles)
* [/sys/devices/platform/ipmi\_si.\*/long\_timeouts](abi-testing.html#abi-sys-devices-platform-ipmi-si-idles)
* [/sys/devices/platform/ipmi\_si.\*/flag\_fetches](abi-testing.html#abi-sys-devices-platform-ipmi-si-idles)
* [/sys/devices/platform/ipmi\_si.\*/attentions](abi-testing.html#abi-sys-devices-platform-ipmi-si-idles)
* [/sys/devices/platform/ipmi\_si.\*/incoming\_messages](abi-testing.html#abi-sys-devices-platform-ipmi-si-idles)
* [/sys/devices/platform/ipmi\_si.\*/short\_timeouts](abi-testing.html#abi-sys-devices-platform-ipmi-si-idles)
* [/sys/devices/platform/ipmi\_si.\*/interrupts\_enabled](abi-testing.html#abi-sys-devices-platform-ipmi-si-interrupts-enabled)
* [/sys/devices/platform/ipmi\_si.\*/params](abi-testing.html#abi-sys-devices-platform-ipmi-si-params)
* [/sys/devices/platform/dmi-ipmi-ssif.\*/type](abi-testing.html#abi-sys-devices-platform-dmi-ipmi-ssif-type)
* [/sys/devices/platform/dmi-ipmi-ssif.\*/hosed](abi-testing.html#abi-sys-devices-platform-dmi-ipmi-ssif-hosed)
* [/sys/devices/platform/dmi-ipmi-ssif.\*/alerts](abi-testing.html#abi-sys-devices-platform-dmi-ipmi-ssif-hosed)
* [/sys/devices/platform/dmi-ipmi-ssif.\*/sent\_messages](abi-testing.html#abi-sys-devices-platform-dmi-ipmi-ssif-hosed)
* [/sys/devices/platform/dmi-ipmi-ssif.\*/sent\_messages\_parts](abi-testing.html#abi-sys-devices-platform-dmi-ipmi-ssif-hosed)
* [/sys/devices/platform/dmi-ipmi-ssif.\*/received\_messages](abi-testing.html#abi-sys-devices-platform-dmi-ipmi-ssif-hosed)
* [/sys/devices/platform/dmi-ipmi-ssif.\*/received\_message\_parts](abi-testing.html#abi-sys-devices-platform-dmi-ipmi-ssif-hosed)
* [/sys/devices/platform/dmi-ipmi-ssif.\*/events](abi-testing.html#abi-sys-devices-platform-dmi-ipmi-ssif-hosed)
* [/sys/devices/platform/dmi-ipmi-ssif.\*/watchdog\_pretimeouts](abi-testing.html#abi-sys-devices-platform-dmi-ipmi-ssif-hosed)
* [/sys/devices/platform/dmi-ipmi-ssif.\*/flag\_fetches](abi-testing.html#abi-sys-devices-platform-dmi-ipmi-ssif-hosed)
* [/sys/devices/platform/dmi-ipmi-ssif.\*/send\_retries](abi-testing.html#abi-sys-devices-platform-dmi-ipmi-ssif-hosed)
* [/sys/devices/platform/dmi-ipmi-ssif.\*/receive\_retries](abi-testing.html#abi-sys-devices-platform-dmi-ipmi-ssif-hosed)
* [/sys/devices/platform/dmi-ipmi-ssif.\*/send\_errors](abi-testing.html#abi-sys-devices-platform-dmi-ipmi-ssif-hosed)
* [/sys/devices/platform/dmi-ipmi-ssif.\*/receive\_errors](abi-testing.html#abi-sys-devices-platform-dmi-ipmi-ssif-hosed)

## ABI file testing/sysfs-devices-platform-kunpeng\_hccs

Has the following ABI:

* [/sys/devices/platform/HISI04Bx:00/chipX/all\_linked](abi-testing.html#abi-sys-devices-platform-hisi04bx-00-chipx-all-linked)
* [/sys/devices/platform/HISI04Bx:00/chipX/linked\_full\_lane](abi-testing.html#abi-sys-devices-platform-hisi04bx-00-chipx-all-linked)
* [/sys/devices/platform/HISI04Bx:00/chipX/crc\_err\_cnt](abi-testing.html#abi-sys-devices-platform-hisi04bx-00-chipx-all-linked)
* [/sys/devices/platform/HISI04Bx:00/chipX/dieY/all\_linked](abi-testing.html#abi-sys-devices-platform-hisi04bx-00-chipx-diey-all-linked)
* [/sys/devices/platform/HISI04Bx:00/chipX/dieY/linked\_full\_lane](abi-testing.html#abi-sys-devices-platform-hisi04bx-00-chipx-diey-all-linked)
* [/sys/devices/platform/HISI04Bx:00/chipX/dieY/crc\_err\_cnt](abi-testing.html#abi-sys-devices-platform-hisi04bx-00-chipx-diey-all-linked)
* [/sys/devices/platform/HISI04Bx:00/chipX/dieY/hccsN/type](abi-testing.html#abi-sys-devices-platform-hisi04bx-00-chipx-diey-hccsn-type)
* [/sys/devices/platform/HISI04Bx:00/chipX/dieY/hccsN/lane\_mode](abi-testing.html#abi-sys-devices-platform-hisi04bx-00-chipx-diey-hccsn-type)
* [/sys/devices/platform/HISI04Bx:00/chipX/dieY/hccsN/enable](abi-testing.html#abi-sys-devices-platform-hisi04bx-00-chipx-diey-hccsn-type)
* [/sys/devices/platform/HISI04Bx:00/chipX/dieY/hccsN/cur\_lane\_num](abi-testing.html#abi-sys-devices-platform-hisi04bx-00-chipx-diey-hccsn-type)
* [/sys/devices/platform/HISI04Bx:00/chipX/dieY/hccsN/link\_fsm](abi-testing.html#abi-sys-devices-platform-hisi04bx-00-chipx-diey-hccsn-type)
* [/sys/devices/platform/HISI04Bx:00/chipX/dieY/hccsN/lane\_mask](abi-testing.html#abi-sys-devices-platform-hisi04bx-00-chipx-diey-hccsn-type)
* [/sys/devices/platform/HISI04Bx:00/chipX/dieY/hccsN/crc\_err\_cnt](abi-testing.html#abi-sys-devices-platform-hisi04bx-00-chipx-diey-hccsn-type)
* [/sys/devices/platform/HISI04Bx:00/used\_types](abi-testing.html#abi-sys-devices-platform-hisi04bx-00-used-types)
* [/sys/devices/platform/HISI04Bx:00/available\_inc\_dec\_lane\_types](abi-testing.html#abi-sys-devices-platform-hisi04bx-00-available-inc-dec-lane-types)
* [/sys/devices/platform/HISI04Bx:00/dec\_lane\_of\_type](abi-testing.html#abi-sys-devices-platform-hisi04bx-00-available-inc-dec-lane-types)
* [/sys/devices/platform/HISI04Bx:00/inc\_lane\_of\_type](abi-testing.html#abi-sys-devices-platform-hisi04bx-00-available-inc-dec-lane-types)

## ABI file testing/sysfs-devices-platform-sh\_mobile\_lcdc\_fb

Has the following ABI:

* [/sys/devices/platform/sh\_mobile\_lcdc\_fb.[0-3]/graphics/fb[0-9]/ovl\_alpha](abi-testing.html#abi-sys-devices-platform-sh-mobile-lcdc-fb-0-3-graphics-fb-0-9-ovl-alpha)
* [/sys/devices/platform/sh\_mobile\_lcdc\_fb.[0-3]/graphics/fb[0-9]/ovl\_mode](abi-testing.html#abi-sys-devices-platform-sh-mobile-lcdc-fb-0-3-graphics-fb-0-9-ovl-mode)
* [/sys/devices/platform/sh\_mobile\_lcdc\_fb.[0-3]/graphics/fb[0-9]/ovl\_position](abi-testing.html#abi-sys-devices-platform-sh-mobile-lcdc-fb-0-3-graphics-fb-0-9-ovl-position)
* [/sys/devices/platform/sh\_mobile\_lcdc\_fb.[0-3]/graphics/fb[0-9]/ovl\_rop3](abi-testing.html#abi-sys-devices-platform-sh-mobile-lcdc-fb-0-3-graphics-fb-0-9-ovl-rop3)

## ABI file testing/sysfs-devices-platform-soc-ipa

Has the following ABI:

* [/sys/devices/platform/soc@X/XXXXXXX.ipa/](abi-testing.html#abi-sys-devices-platform-soc-x-xxxxxxx-ipa)
* [.../XXXXXXX.ipa/version](abi-testing.html#abi-xxxxxxx-ipa-version)
* [.../XXXXXXX.ipa/feature/](abi-testing.html#abi-xxxxxxx-ipa-feature)
* [.../XXXXXXX.ipa/feature/rx\_offload](abi-testing.html#abi-xxxxxxx-ipa-feature-rx-offload)
* [.../XXXXXXX.ipa/feature/tx\_offload](abi-testing.html#abi-xxxxxxx-ipa-feature-tx-offload)
* [.../XXXXXXX.ipa/endpoint\_id/](abi-testing.html#abi-xxxxxxx-ipa-endpoint-id)
* [.../XXXXXXX.ipa/endpoint\_id/modem\_rx](abi-testing.html#abi-xxxxxxx-ipa-endpoint-id-modem-rx)
* [.../XXXXXXX.ipa/endpoint\_id/modem\_tx](abi-testing.html#abi-xxxxxxx-ipa-endpoint-id-modem-tx)
* [.../XXXXXXX.ipa/endpoint\_id/monitor\_rx](abi-testing.html#abi-xxxxxxx-ipa-endpoint-id-monitor-rx)
* [.../XXXXXXX.ipa/modem/](abi-testing.html#abi-xxxxxxx-ipa-modem)
* [.../XXXXXXX.ipa/modem/rx\_endpoint\_id](abi-testing.html#abi-xxxxxxx-ipa-modem-rx-endpoint-id)
* [.../XXXXXXX.ipa/modem/tx\_endpoint\_id](abi-testing.html#abi-xxxxxxx-ipa-modem-tx-endpoint-id)

## ABI file testing/sysfs-devices-platform-stratix10-rsu

> Intel Stratix10 Remote System Update (RSU) device attributes

Has the following ABI:

* [/sys/devices/platform/stratix10-rsu.0/current\_image](abi-testing.html#abi-sys-devices-platform-stratix10-rsu-0-current-image)
* [/sys/devices/platform/stratix10-rsu.0/fail\_image](abi-testing.html#abi-sys-devices-platform-stratix10-rsu-0-fail-image)
* [/sys/devices/platform/stratix10-rsu.0/state](abi-testing.html#abi-sys-devices-platform-stratix10-rsu-0-state)
* [/sys/devices/platform/stratix10-rsu.0/version](abi-testing.html#abi-sys-devices-platform-stratix10-rsu-0-version)
* [/sys/devices/platform/stratix10-rsu.0/error\_location](abi-testing.html#abi-sys-devices-platform-stratix10-rsu-0-error-location)
* [/sys/devices/platform/stratix10-rsu.0/error\_details](abi-testing.html#abi-sys-devices-platform-stratix10-rsu-0-error-details)
* [/sys/devices/platform/stratix10-rsu.0/retry\_counter](abi-testing.html#abi-sys-devices-platform-stratix10-rsu-0-retry-counter)
* [/sys/devices/platform/stratix10-rsu.0/reboot\_image](abi-testing.html#abi-sys-devices-platform-stratix10-rsu-0-reboot-image)
* [/sys/devices/platform/stratix10-rsu.0/notify](abi-testing.html#abi-sys-devices-platform-stratix10-rsu-0-notify)
* [/sys/devices/platform/stratix10-rsu.0/dcmf0](abi-testing.html#abi-sys-devices-platform-stratix10-rsu-0-dcmf0)
* [/sys/devices/platform/stratix10-rsu.0/dcmf1](abi-testing.html#abi-sys-devices-platform-stratix10-rsu-0-dcmf1)
* [/sys/devices/platform/stratix10-rsu.0/dcmf2](abi-testing.html#abi-sys-devices-platform-stratix10-rsu-0-dcmf2)
* [/sys/devices/platform/stratix10-rsu.0/dcmf3](abi-testing.html#abi-sys-devices-platform-stratix10-rsu-0-dcmf3)
* [/sys/devices/platform/stratix10-rsu.0/max\_retry](abi-testing.html#abi-sys-devices-platform-stratix10-rsu-0-max-retry)

## ABI file testing/sysfs-devices-platform-trackpoint

Has the following ABI:

* [/sys/devices/platform/i8042/.../sensitivity](abi-testing.html#abi-sys-devices-platform-i8042-sensitivity)
* [/sys/devices/platform/i8042/.../intertia](abi-testing.html#abi-sys-devices-platform-i8042-intertia)
* [/sys/devices/platform/i8042/.../reach](abi-testing.html#abi-sys-devices-platform-i8042-reach)
* [/sys/devices/platform/i8042/.../draghys](abi-testing.html#abi-sys-devices-platform-i8042-draghys)
* [/sys/devices/platform/i8042/.../mindrag](abi-testing.html#abi-sys-devices-platform-i8042-mindrag)
* [/sys/devices/platform/i8042/.../speed](abi-testing.html#abi-sys-devices-platform-i8042-speed)
* [/sys/devices/platform/i8042/.../thresh](abi-testing.html#abi-sys-devices-platform-i8042-thresh)
* [/sys/devices/platform/i8042/.../upthresh](abi-testing.html#abi-sys-devices-platform-i8042-upthresh)
* [/sys/devices/platform/i8042/.../ztime](abi-testing.html#abi-sys-devices-platform-i8042-ztime)
* [/sys/devices/platform/i8042/.../jenks](abi-testing.html#abi-sys-devices-platform-i8042-jenks)
* [/sys/devices/platform/i8042/.../skipback](abi-testing.html#abi-sys-devices-platform-i8042-skipback)
* [/sys/devices/platform/i8042/.../ext\_dev](abi-testing.html#abi-sys-devices-platform-i8042-ext-dev)
* [/sys/devices/platform/i8042/.../press\_to\_select](abi-testing.html#abi-sys-devices-platform-i8042-press-to-select)
* [/sys/devices/platform/i8042/.../drift\_time](abi-testing.html#abi-sys-devices-platform-i8042-drift-time)

## ABI file testing/sysfs-devices-power

Has the following ABI:

* [/sys/devices/.../power/](abi-testing.html#abi-sys-devices-power)
* [/sys/devices/.../power/wakeup](abi-testing.html#abi-sys-devices-power-wakeup)
* [/sys/devices/.../power/control](abi-testing.html#abi-sys-devices-power-control)
* [/sys/devices/.../power/async](abi-testing.html#abi-sys-devices-power-async)
* [/sys/devices/.../power/wakeup\_count](abi-testing.html#abi-sys-devices-power-wakeup-count)
* [/sys/devices/.../power/wakeup\_active\_count](abi-testing.html#abi-sys-devices-power-wakeup-active-count)
* [/sys/devices/.../power/wakeup\_abort\_count](abi-testing.html#abi-sys-devices-power-wakeup-abort-count)
* [/sys/devices/.../power/wakeup\_expire\_count](abi-testing.html#abi-sys-devices-power-wakeup-expire-count)
* [/sys/devices/.../power/wakeup\_active](abi-testing.html#abi-sys-devices-power-wakeup-active)
* [/sys/devices/.../power/wakeup\_total\_time\_ms](abi-testing.html#abi-sys-devices-power-wakeup-total-time-ms)
* [/sys/devices/.../power/wakeup\_max\_time\_ms](abi-testing.html#abi-sys-devices-power-wakeup-max-time-ms)
* [/sys/devices/.../power/wakeup\_last\_time\_ms](abi-testing.html#abi-sys-devices-power-wakeup-last-time-ms)
* [/sys/devices/.../power/wakeup\_prevent\_sleep\_time\_ms](abi-testing.html#abi-sys-devices-power-wakeup-prevent-sleep-time-ms)
* [/sys/devices/.../power/autosuspend\_delay\_ms](abi-testing.html#abi-sys-devices-power-autosuspend-delay-ms)
* [/sys/devices/.../power/pm\_qos\_resume\_latency\_us](abi-testing.html#abi-sys-devices-power-pm-qos-resume-latency-us)
* [/sys/devices/.../power/pm\_qos\_latency\_tolerance\_us](abi-testing.html#abi-sys-devices-power-pm-qos-latency-tolerance-us)
* [/sys/devices/.../power/pm\_qos\_no\_power\_off](abi-testing.html#abi-sys-devices-power-pm-qos-no-power-off)
* [/sys/devices/.../power/runtime\_status](abi-testing.html#abi-sys-devices-power-runtime-status)
* [/sys/devices/.../power/runtime\_active\_time](abi-testing.html#abi-sys-devices-power-runtime-active-time)
* [/sys/devices/.../power/runtime\_suspended\_time](abi-testing.html#abi-sys-devices-power-runtime-suspended-time)
* [/sys/devices/.../power/runtime\_usage](abi-testing.html#abi-sys-devices-power-runtime-usage)
* [/sys/devices/.../power/runtime\_enabled](abi-testing.html#abi-sys-devices-power-runtime-enabled)
* [/sys/devices/.../power/runtime\_active\_kids](abi-testing.html#abi-sys-devices-power-runtime-active-kids)

## ABI file testing/sysfs-devices-power\_resources\_D0

Has the following ABI:

* [/sys/devices/.../power\_resources\_D0/](abi-testing.html#abi-sys-devices-power-resources-d0)

## ABI file testing/sysfs-devices-power\_resources\_D1

Has the following ABI:

* [/sys/devices/.../power\_resources\_D1/](abi-testing.html#abi-sys-devices-power-resources-d1)

## ABI file testing/sysfs-devices-power\_resources\_D2

Has the following ABI:

* [/sys/devices/.../power\_resources\_D2/](abi-testing.html#abi-sys-devices-power-resources-d2)

## ABI file testing/sysfs-devices-power\_resources\_D3hot

Has the following ABI:

* [/sys/devices/.../power\_resources\_D3hot/](abi-testing.html#abi-sys-devices-power-resources-d3hot)

## ABI file testing/sysfs-devices-power\_resources\_wakeup

Has the following ABI:

* [/sys/devices/.../power\_resources\_wakeup/](abi-testing.html#abi-sys-devices-power-resources-wakeup)

## ABI file testing/sysfs-devices-power\_state

Has the following ABI:

* [/sys/devices/.../power\_state](abi-testing.html#abi-sys-devices-power-state)

## ABI file testing/sysfs-devices-real\_power\_state

Has the following ABI:

* [/sys/devices/.../real\_power\_state](abi-testing.html#abi-sys-devices-real-power-state)

## ABI file testing/sysfs-devices-removable

Has the following ABI:

* [/sys/devices/.../removable](abi-testing.html#abi-sys-devices-removable)

## ABI file testing/sysfs-devices-resource\_in\_use

Has the following ABI:

* [/sys/devices/.../resource\_in\_use](abi-testing.html#abi-sys-devices-resource-in-use)

## ABI file testing/sysfs-devices-soc

Has the following ABI:

* [/sys/devices/socX](abi-testing.html#abi-sys-devices-socx)
* [/sys/devices/socX/machine](abi-testing.html#abi-sys-devices-socx-machine)
* [/sys/devices/socX/family](abi-testing.html#abi-sys-devices-socx-family)
* [/sys/devices/socX/serial\_number](abi-testing.html#abi-sys-devices-socx-serial-number)
* [/sys/devices/socX/soc\_id](abi-testing.html#abi-sys-devices-socx-soc-id)
* [/sys/devices/socX/revision](abi-testing.html#abi-sys-devices-socx-revision)
* [/sys/devices/socX/process](abi-testing.html#abi-sys-devices-socx-process)
* [/sys/bus/soc](abi-testing.html#abi-sys-bus-soc)

## ABI file testing/sysfs-devices-software\_node

Has the following ABI:

* [/sys/devices/.../software\_node/](abi-testing.html#abi-sys-devices-software-node)

## ABI file testing/sysfs-devices-state\_synced

Has the following ABI:

* [/sys/devices/.../state\_synced](abi-testing.html#abi-sys-devices-state-synced)

## ABI file testing/sysfs-devices-sun

Has the following ABI:

* [/sys/devices/.../sun](abi-testing.html#abi-sys-devices-sun)

## ABI file testing/sysfs-devices-supplier

Has the following ABI:

* [/sys/devices/.../supplier:<supplier>](abi-testing.html#abi-sys-devices-supplier-supplier)

## ABI file testing/sysfs-devices-system-cpu

Has the following ABI:

* [/sys/devices/system/cpu/](abi-testing.html#abi-sys-devices-system-cpu)
* [/sys/devices/system/cpu/kernel\_max](abi-testing.html#abi-sys-devices-system-cpu-kernel-max)
* [/sys/devices/system/cpu/probe](abi-testing.html#abi-sys-devices-system-cpu-probe)
* [/sys/devices/system/cpu/cpuX/node](abi-testing.html#abi-sys-devices-system-cpu-cpux-node)
* [/sys/devices/system/cpu/cpuX/topology/core\_siblings](abi-testing.html#abi-sys-devices-system-cpu-cpux-topology-core-siblings)
* [/sys/devices/system/cpu/cpuidle/available\_governors](abi-testing.html#abi-sys-devices-system-cpu-cpuidle-available-governors)
* [/sys/devices/system/cpu/cpuX/cpuidle/state<N>/name](abi-testing.html#abi-sys-devices-system-cpu-cpux-cpuidle-state-n-name)
* [/sys/devices/system/cpu/cpuX/cpuidle/state<N>/desc](abi-testing.html#abi-sys-devices-system-cpu-cpux-cpuidle-state-n-desc)
* [/sys/devices/system/cpu/cpuX/cpuidle/state<N>/disable](abi-testing.html#abi-sys-devices-system-cpu-cpux-cpuidle-state-n-disable)
* [/sys/devices/system/cpu/cpuX/cpuidle/state<N>/default\_status](abi-testing.html#abi-sys-devices-system-cpu-cpux-cpuidle-state-n-default-status)
* [/sys/devices/system/cpu/cpuX/cpuidle/state<N>/residency](abi-testing.html#abi-sys-devices-system-cpu-cpux-cpuidle-state-n-residency)
* [/sys/devices/system/cpu/cpuX/cpuidle/state<N>/s2idle/](abi-testing.html#abi-sys-devices-system-cpu-cpux-cpuidle-state-n-s2idle)
* [/sys/devices/system/cpu/cpuX/cpuidle/state<N>/s2idle/time](abi-testing.html#abi-sys-devices-system-cpu-cpux-cpuidle-state-n-s2idle-time)
* [/sys/devices/system/cpu/cpuX/cpuidle/state<N>/s2idle/usage](abi-testing.html#abi-sys-devices-system-cpu-cpux-cpuidle-state-n-s2idle-usage)
* [/sys/devices/system/cpu/cpuX/cpufreq/\*](abi-testing.html#abi-sys-devices-system-cpu-cpux-cpufreq)
* [/sys/devices/system/cpu/cpuX/cpufreq/freqdomain\_cpus](abi-testing.html#abi-sys-devices-system-cpu-cpux-cpufreq-freqdomain-cpus)
* [/sys/devices/system/cpu/cpuX/cpufreq/auto\_select](abi-testing.html#abi-sys-devices-system-cpu-cpux-cpufreq-auto-select)
* [/sys/devices/system/cpu/cpuX/cpufreq/auto\_act\_window](abi-testing.html#abi-sys-devices-system-cpu-cpux-cpufreq-auto-act-window)
* [/sys/devices/system/cpu/cpuX/cpufreq/energy\_performance\_preference\_val](abi-testing.html#abi-sys-devices-system-cpu-cpux-cpufreq-energy-performance-preference-val)
* [/sys/devices/system/cpu/cpuX/cpufreq/perf\_limited](abi-testing.html#abi-sys-devices-system-cpu-cpux-cpufreq-perf-limited)
* [/sys/devices/system/cpu/cpu\*/cache/index3/cache\_disable\_{0,1}](abi-testing.html#abi-sys-devices-system-cpu-cpu-cache-index3-cache-disable-0-1)
* [/sys/devices/system/cpu/cpufreq/boost](abi-testing.html#abi-sys-devices-system-cpu-cpufreq-boost)
* [/sys/devices/system/cpu/cpuX/crash\_notes](abi-testing.html#abi-sys-devices-system-cpu-cpux-crash-notes)
* [/sys/devices/system/cpu/intel\_pstate/max\_perf\_pct](abi-testing.html#abi-sys-devices-system-cpu-intel-pstate-max-perf-pct)
* [/sys/devices/system/cpu/cpu\*/cache/index\*/<set\_of\_attributes\_mentioned\_below>](abi-testing.html#abi-sys-devices-system-cpu-cpu-cache-index-set-of-attributes-mentioned-below)
* [/sys/devices/system/cpu/cpu\*/cache/index\*/id](abi-testing.html#abi-sys-devices-system-cpu-cpu-cache-index-id)
* [/sys/devices/system/cpu/cpuX/cpufreq/throttle\_stats](abi-testing.html#abi-sys-devices-system-cpu-cpux-cpufreq-throttle-stats)
* [/sys/devices/system/cpu/cpufreq/policyX/throttle\_stats](abi-testing.html#abi-sys-devices-system-cpu-cpufreq-policyx-throttle-stats)
* [/sys/devices/system/cpu/cpuX/regs/](abi-testing.html#abi-sys-devices-system-cpu-cpux-regs)
* [/sys/devices/system/cpu/aarch32\_el0](abi-testing.html#abi-sys-devices-system-cpu-aarch32-el0)
* [/sys/devices/system/cpu/cpuX/cpu\_capacity](abi-testing.html#abi-sys-devices-system-cpu-cpux-cpu-capacity)
* [/sys/devices/system/cpu/vulnerabilities](abi-testing.html#abi-sys-devices-system-cpu-vulnerabilities)
* [/sys/devices/system/cpu/smt](abi-testing.html#abi-sys-devices-system-cpu-smt)
* [/sys/devices/system/cpu/cpuX/power/energy\_perf\_bias](abi-testing.html#abi-sys-devices-system-cpu-cpux-power-energy-perf-bias)
* [/sys/devices/system/cpu/umwait\_control](abi-testing.html#abi-sys-devices-system-cpu-umwait-control)
* [/sys/devices/system/cpu/sev](abi-testing.html#abi-sys-devices-system-cpu-sev)
* [/sys/devices/system/cpu/svm](abi-testing.html#abi-sys-devices-system-cpu-svm)
* [/sys/devices/system/cpu/cpuX/purr](abi-testing.html#abi-sys-devices-system-cpu-cpux-purr)
* [/sys/devices/system/cpu/cpuX/spurr](abi-testing.html#abi-sys-devices-system-cpu-cpux-spurr)
* [/sys/devices/system/cpu/cpuX/idle\_purr](abi-testing.html#abi-sys-devices-system-cpu-cpux-idle-purr)
* [/sys/devices/system/cpu/cpuX/idle\_spurr](abi-testing.html#abi-sys-devices-system-cpu-cpux-idle-spurr)
* [/sys/devices/system/cpu/cpuX/mte\_tcf\_preferred](abi-testing.html#abi-sys-devices-system-cpu-cpux-mte-tcf-preferred)
* [/sys/devices/system/cpu/nohz\_full](abi-testing.html#abi-sys-devices-system-cpu-nohz-full)
* [/sys/devices/system/cpu/isolated](abi-testing.html#abi-sys-devices-system-cpu-isolated)
* [/sys/devices/system/cpu/housekeeping](abi-testing.html#abi-sys-devices-system-cpu-housekeeping)
* [/sys/devices/system/cpu/crash\_hotplug](abi-testing.html#abi-sys-devices-system-cpu-crash-hotplug)
* [/sys/devices/system/cpu/enabled](abi-testing.html#abi-sys-devices-system-cpu-enabled)

## ABI file testing/sysfs-devices-system-ibm-rtl

Has the following ABI:

* [/sys/devices/system/ibm\_rtl/state](abi-testing.html#abi-sys-devices-system-ibm-rtl-state)
* [/sys/devices/system/ibm\_rtl/version](abi-testing.html#abi-sys-devices-system-ibm-rtl-version)

## ABI file testing/sysfs-devices-system-xen\_cpu

Has the following ABI:

* [/sys/devices/system/xen\_cpu/](abi-testing.html#abi-sys-devices-system-xen-cpu)
* [/sys/devices/system/xen\_cpu/xen\_cpu#/online](abi-testing.html#abi-sys-devices-system-xen-cpu-xen-cpu-online)

## ABI file testing/sysfs-devices-vfio-dev

Has the following ABI:

* [/sys/.../<device>/vfio-dev/vfioX/](abi-testing.html#abi-sys-device-vfio-dev-vfiox)

## ABI file testing/sysfs-devices-virtual-misc-tdx\_guest

Has the following ABI:

* [/sys/devices/virtual/misc/tdx\_guest/measurements/MRNAME[:HASH]](abi-testing.html#abi-sys-devices-virtual-misc-tdx-guest-measurements-mrname-hash)
* [/sys/devices/virtual/misc/tdx\_guest/measurements/mrconfigid](abi-testing.html#abi-sys-devices-virtual-misc-tdx-guest-measurements-mrconfigid)
* [/sys/devices/virtual/misc/tdx\_guest/measurements/mrowner](abi-testing.html#abi-sys-devices-virtual-misc-tdx-guest-measurements-mrowner)
* [/sys/devices/virtual/misc/tdx\_guest/measurements/mrownerconfig](abi-testing.html#abi-sys-devices-virtual-misc-tdx-guest-measurements-mrownerconfig)
* [/sys/devices/virtual/misc/tdx\_guest/measurements/mrtd:sha384](abi-testing.html#abi-sys-devices-virtual-misc-tdx-guest-measurements-mrtd-sha384)
* [/sys/devices/virtual/misc/tdx\_guest/measurements/rtmr[0123]:sha384](abi-testing.html#abi-sys-devices-virtual-misc-tdx-guest-measurements-rtmr-0123-sha384)

## ABI file testing/sysfs-devices-waiting\_for\_supplier

Has the following ABI:

* [/sys/devices/.../waiting\_for\_supplier](abi-testing.html#abi-sys-devices-waiting-for-supplier)

## ABI file testing/sysfs-devices-xenbus

Has the following ABI:

* [/sys/devices/\*/xenbus/event\_channels](abi-testing.html#abi-sys-devices-xenbus-event-channels)
* [/sys/devices/\*/xenbus/events](abi-testing.html#abi-sys-devices-xenbus-events)
* [/sys/devices/\*/xenbus/jiffies\_eoi\_delayed](abi-testing.html#abi-sys-devices-xenbus-jiffies-eoi-delayed)
* [/sys/devices/\*/xenbus/spurious\_events](abi-testing.html#abi-sys-devices-xenbus-spurious-events)
* [/sys/devices/\*/xenbus/spurious\_threshold](abi-testing.html#abi-sys-devices-xenbus-spurious-threshold)

## ABI file testing/sysfs-driver-altera-cvp

Has the following ABI:

* [/sys/bus/pci/drivers/altera-cvp/chkcfg](abi-testing.html#abi-sys-bus-pci-drivers-altera-cvp-chkcfg)

## ABI file testing/sysfs-driver-amd-sfh

Has the following ABI:

* [/sys/bus/pci/drivers/pcie\_mp2\_amd/\*/hpd](abi-testing.html#abi-sys-bus-pci-drivers-pcie-mp2-amd-hpd)

## ABI file testing/sysfs-driver-aspeed-uart-routing

Has the following ABI:

* [/sys/bus/platform/drivers/aspeed-uart-routing/\\*/uart\\*](abi-testing.html#abi-sys-bus-platform-drivers-aspeed-uart-routing-uart)
* [/sys/bus/platform/drivers/aspeed-uart-routing/\\*/io\\*](abi-testing.html#abi-sys-bus-platform-drivers-aspeed-uart-routing-io)

## ABI file testing/sysfs-driver-bd9571mwv-regulator

Has the following ABI:

* [/sys/bus/i2c/devices/.../bd9571mwv-regulator.\*.auto/backup\_mode](abi-testing.html#abi-sys-bus-i2c-devices-bd9571mwv-regulator-auto-backup-mode)

## ABI file testing/sysfs-driver-ccp

Has the following ABI:

* [/sys/bus/pci/devices/<BDF>/fused\_part](abi-testing.html#abi-sys-bus-pci-devices-bdf-fused-part)
* [/sys/bus/pci/devices/<BDF>/boot\_integrity](abi-testing.html#abi-sys-bus-pci-devices-bdf-boot-integrity)
* [/sys/bus/pci/devices/<BDF>/debug\_lock\_on](abi-testing.html#abi-sys-bus-pci-devices-bdf-debug-lock-on)
* [/sys/bus/pci/devices/<BDF>/tsme\_status](abi-testing.html#abi-sys-bus-pci-devices-bdf-tsme-status)
* [/sys/bus/pci/devices/<BDF>/anti\_rollback\_status](abi-testing.html#abi-sys-bus-pci-devices-bdf-anti-rollback-status)
* [/sys/bus/pci/devices/<BDF>/rpmc\_production\_enabled](abi-testing.html#abi-sys-bus-pci-devices-bdf-rpmc-production-enabled)
* [/sys/bus/pci/devices/<BDF>/rpmc\_spirom\_available](abi-testing.html#abi-sys-bus-pci-devices-bdf-rpmc-spirom-available)
* [/sys/bus/pci/devices/<BDF>/hsp\_tpm\_available](abi-testing.html#abi-sys-bus-pci-devices-bdf-hsp-tpm-available)
* [/sys/bus/pci/devices/<BDF>/rom\_armor\_enforced](abi-testing.html#abi-sys-bus-pci-devices-bdf-rom-armor-enforced)
* [/sys/bus/pci/devices/<BDF>/bootloader\_version](abi-testing.html#abi-sys-bus-pci-devices-bdf-bootloader-version)
* [/sys/bus/pci/devices/<BDF>/tee\_version](abi-testing.html#abi-sys-bus-pci-devices-bdf-tee-version)

## ABI file testing/sysfs-driver-chromeos-acpi

Has the following ABI:

* [/sys/bus/platform/devices/GGL0001:\*/BINF.2](abi-testing.html#abi-sys-bus-platform-devices-ggl0001-binf-2)
* [/sys/bus/platform/devices/GGL0001:\*/BINF.3](abi-testing.html#abi-sys-bus-platform-devices-ggl0001-binf-3)
* [/sys/bus/platform/devices/GGL0001:\*/CHSW](abi-testing.html#abi-sys-bus-platform-devices-ggl0001-chsw)
* [/sys/bus/platform/devices/GGL0001:\*/FMAP](abi-testing.html#abi-sys-bus-platform-devices-ggl0001-fmap)
* [/sys/bus/platform/devices/GGL0001:\*/FRID](abi-testing.html#abi-sys-bus-platform-devices-ggl0001-frid)
* [/sys/bus/platform/devices/GGL0001:\*/FWID](abi-testing.html#abi-sys-bus-platform-devices-ggl0001-fwid)
* [/sys/bus/platform/devices/GGL0001:\*/GPIO.X/GPIO.0](abi-testing.html#abi-sys-bus-platform-devices-ggl0001-gpio-x-gpio-0)
* [/sys/bus/platform/devices/GGL0001:\*/GPIO.X/GPIO.1](abi-testing.html#abi-sys-bus-platform-devices-ggl0001-gpio-x-gpio-1)
* [/sys/bus/platform/devices/GGL0001:\*/GPIO.X/GPIO.2](abi-testing.html#abi-sys-bus-platform-devices-ggl0001-gpio-x-gpio-2)
* [/sys/bus/platform/devices/GGL0001:\*/GPIO.X/GPIO.3](abi-testing.html#abi-sys-bus-platform-devices-ggl0001-gpio-x-gpio-3)
* [/sys/bus/platform/devices/GGL0001:\*/HWID](abi-testing.html#abi-sys-bus-platform-devices-ggl0001-hwid)
* [/sys/bus/platform/devices/GGL0001:\*/MECK](abi-testing.html#abi-sys-bus-platform-devices-ggl0001-meck)
* [/sys/bus/platform/devices/GGL0001:\*/VBNV.0](abi-testing.html#abi-sys-bus-platform-devices-ggl0001-vbnv-0)
* [/sys/bus/platform/devices/GGL0001:\*/VBNV.1](abi-testing.html#abi-sys-bus-platform-devices-ggl0001-vbnv-1)
* [/sys/bus/platform/devices/GGL0001:\*/VDAT](abi-testing.html#abi-sys-bus-platform-devices-ggl0001-vdat)

## ABI file testing/sysfs-driver-eud

Has the following ABI:

* [/sys/bus/platform/drivers/qcom\_eud/.../enable](abi-testing.html#abi-sys-bus-platform-drivers-qcom-eud-enable)

## ABI file testing/sysfs-driver-framer-pef2256

Has the following ABI:

* [/sys/bus/platform/devices/xxx/version](abi-testing.html#abi-sys-bus-platform-devices-xxx-version)

## ABI file testing/sysfs-driver-fsi-master-gpio

Has the following ABI:

* [/sys/bus/platform/devices/[..]/fsi-master-gpio/external\_mode](abi-testing.html#abi-sys-bus-platform-devices-fsi-master-gpio-external-mode)

## ABI file testing/sysfs-driver-ge-achc

Has the following ABI:

* [/sys/bus/spi/<dev>/update\_firmware](abi-testing.html#abi-sys-bus-spi-dev-update-firmware)
* [/sys/bus/spi/<dev>/reset](abi-testing.html#abi-sys-bus-spi-dev-reset)

## ABI file testing/sysfs-driver-genwqe

Has the following ABI:

* [/sys/class/genwqe/genwqe<n>\_card/version](abi-testing.html#abi-sys-class-genwqe-genwqe-n-card-version)
* [/sys/class/genwqe/genwqe<n>\_card/appid](abi-testing.html#abi-sys-class-genwqe-genwqe-n-card-appid)
* [/sys/class/genwqe/genwqe<n>\_card/type](abi-testing.html#abi-sys-class-genwqe-genwqe-n-card-type)
* [/sys/class/genwqe/genwqe<n>\_card/curr\_bitstream](abi-testing.html#abi-sys-class-genwqe-genwqe-n-card-curr-bitstream)
* [/sys/class/genwqe/genwqe<n>\_card/next\_bitstream](abi-testing.html#abi-sys-class-genwqe-genwqe-n-card-next-bitstream)
* [/sys/class/genwqe/genwqe<n>\_card/reload\_bitstream](abi-testing.html#abi-sys-class-genwqe-genwqe-n-card-reload-bitstream)
* [/sys/class/genwqe/genwqe<n>\_card/tempsens](abi-testing.html#abi-sys-class-genwqe-genwqe-n-card-tempsens)
* [/sys/class/genwqe/genwqe<n>\_card/freerunning\_timer](abi-testing.html#abi-sys-class-genwqe-genwqe-n-card-freerunning-timer)
* [/sys/class/genwqe/genwqe<n>\_card/queue\_working\_time](abi-testing.html#abi-sys-class-genwqe-genwqe-n-card-queue-working-time)
* [/sys/class/genwqe/genwqe<n>\_card/state](abi-testing.html#abi-sys-class-genwqe-genwqe-n-card-state)
* [/sys/class/genwqe/genwqe<n>\_card/base\_clock](abi-testing.html#abi-sys-class-genwqe-genwqe-n-card-base-clock)
* [/sys/class/genwqe/genwqe<n>\_card/device/sriov\_numvfs](abi-testing.html#abi-sys-class-genwqe-genwqe-n-card-device-sriov-numvfs)

## ABI file testing/sysfs-driver-habanalabs

Has the following ABI:

* [/sys/class/accel/accel<n>/device/armcp\_kernel\_ver](abi-testing.html#abi-sys-class-accel-accel-n-device-armcp-kernel-ver)
* [/sys/class/accel/accel<n>/device/armcp\_ver](abi-testing.html#abi-sys-class-accel-accel-n-device-armcp-ver)
* [/sys/class/accel/accel<n>/device/clk\_max\_freq\_mhz](abi-testing.html#abi-sys-class-accel-accel-n-device-clk-max-freq-mhz)
* [/sys/class/accel/accel<n>/device/clk\_cur\_freq\_mhz](abi-testing.html#abi-sys-class-accel-accel-n-device-clk-cur-freq-mhz)
* [/sys/class/accel/accel<n>/device/cpld\_ver](abi-testing.html#abi-sys-class-accel-accel-n-device-cpld-ver)
* [/sys/class/accel/accel<n>/device/cpucp\_kernel\_ver](abi-testing.html#abi-sys-class-accel-accel-n-device-cpucp-kernel-ver)
* [/sys/class/accel/accel<n>/device/cpucp\_ver](abi-testing.html#abi-sys-class-accel-accel-n-device-cpucp-ver)
* [/sys/class/accel/accel<n>/device/device\_type](abi-testing.html#abi-sys-class-accel-accel-n-device-device-type)
* [/sys/class/accel/accel<n>/device/eeprom](abi-testing.html#abi-sys-class-accel-accel-n-device-eeprom)
* [/sys/class/accel/accel<n>/device/fuse\_ver](abi-testing.html#abi-sys-class-accel-accel-n-device-fuse-ver)
* [/sys/class/accel/accel<n>/device/fw\_os\_ver](abi-testing.html#abi-sys-class-accel-accel-n-device-fw-os-ver)
* [/sys/class/accel/accel<n>/device/hard\_reset](abi-testing.html#abi-sys-class-accel-accel-n-device-hard-reset)
* [/sys/class/accel/accel<n>/device/hard\_reset\_cnt](abi-testing.html#abi-sys-class-accel-accel-n-device-hard-reset-cnt)
* [/sys/class/accel/accel<n>/device/high\_pll](abi-testing.html#abi-sys-class-accel-accel-n-device-high-pll)
* [/sys/class/accel/accel<n>/device/ic\_clk](abi-testing.html#abi-sys-class-accel-accel-n-device-ic-clk)
* [/sys/class/accel/accel<n>/device/ic\_clk\_curr](abi-testing.html#abi-sys-class-accel-accel-n-device-ic-clk-curr)
* [/sys/class/accel/accel<n>/device/infineon\_ver](abi-testing.html#abi-sys-class-accel-accel-n-device-infineon-ver)
* [/sys/class/accel/accel<n>/device/max\_power](abi-testing.html#abi-sys-class-accel-accel-n-device-max-power)
* [/sys/class/accel/accel<n>/device/mme\_clk](abi-testing.html#abi-sys-class-accel-accel-n-device-mme-clk)
* [/sys/class/accel/accel<n>/device/mme\_clk\_curr](abi-testing.html#abi-sys-class-accel-accel-n-device-mme-clk-curr)
* [/sys/class/accel/accel<n>/device/module\_id](abi-testing.html#abi-sys-class-accel-accel-n-device-module-id)
* [/sys/class/accel/accel<n>/device/parent\_device](abi-testing.html#abi-sys-class-accel-accel-n-device-parent-device)
* [/sys/class/accel/accel<n>/device/pci\_addr](abi-testing.html#abi-sys-class-accel-accel-n-device-pci-addr)
* [/sys/class/accel/accel<n>/device/pm\_mng\_profile](abi-testing.html#abi-sys-class-accel-accel-n-device-pm-mng-profile)
* [/sys/class/accel/accel<n>/device/preboot\_btl\_ver](abi-testing.html#abi-sys-class-accel-accel-n-device-preboot-btl-ver)
* [/sys/class/accel/accel<n>/device/security\_enabled](abi-testing.html#abi-sys-class-accel-accel-n-device-security-enabled)
* [/sys/class/accel/accel<n>/device/soft\_reset](abi-testing.html#abi-sys-class-accel-accel-n-device-soft-reset)
* [/sys/class/accel/accel<n>/device/soft\_reset\_cnt](abi-testing.html#abi-sys-class-accel-accel-n-device-soft-reset-cnt)
* [/sys/class/accel/accel<n>/device/status](abi-testing.html#abi-sys-class-accel-accel-n-device-status)
* [/sys/class/accel/accel<n>/device/thermal\_ver](abi-testing.html#abi-sys-class-accel-accel-n-device-thermal-ver)
* [/sys/class/accel/accel<n>/device/tpc\_clk](abi-testing.html#abi-sys-class-accel-accel-n-device-tpc-clk)
* [/sys/class/accel/accel<n>/device/tpc\_clk\_curr](abi-testing.html#abi-sys-class-accel-accel-n-device-tpc-clk-curr)
* [/sys/class/accel/accel<n>/device/uboot\_ver](abi-testing.html#abi-sys-class-accel-accel-n-device-uboot-ver)
* [/sys/class/accel/accel<n>/device/vrm\_ver](abi-testing.html#abi-sys-class-accel-accel-n-device-vrm-ver)

## ABI file testing/sysfs-driver-hid

Has the following ABI:

* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/report\_descriptor](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-report-descriptor)
* [/sys/class/bluetooth/hci<addr>/<hid-bus>:<vendor-id>:<product-id>.<num>/report\_descriptor](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-report-descriptor)
* [/sys/class/hidraw/hidraw<num>/device/report\_descriptor](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-report-descriptor)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/country](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-country)
* [/sys/class/bluetooth/hci<addr>/<hid-bus>:<vendor-id>:<product-id>.<num>/country](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-country)
* [/sys/class/hidraw/hidraw<num>/device/country](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-country)

## ABI file testing/sysfs-driver-hid-appletb-kbd

Has the following ABI:

* [/sys/bus/hid/drivers/hid-appletb-kbd/<dev>/mode](abi-testing.html#abi-sys-bus-hid-drivers-hid-appletb-kbd-dev-mode)

## ABI file testing/sysfs-driver-hid-corsair

Has the following ABI:

* [/sys/bus/drivers/corsair/<dev>/macro\_mode](abi-testing.html#abi-sys-bus-drivers-corsair-dev-macro-mode)
* [/sys/bus/drivers/corsair/<dev>/current\_profile](abi-testing.html#abi-sys-bus-drivers-corsair-dev-current-profile)

## ABI file testing/sysfs-driver-hid-corsair-void

Has the following ABI:

* [/sys/bus/hid/drivers/hid-corsair-void/<dev>/fw\_version\_headset](abi-testing.html#abi-sys-bus-hid-drivers-hid-corsair-void-dev-fw-version-headset)
* [/sys/bus/hid/drivers/hid-corsair-void/<dev>/fw\_version\_receiver](abi-testing.html#abi-sys-bus-hid-drivers-hid-corsair-void-dev-fw-version-receiver)
* [/sys/bus/hid/drivers/hid-corsair-void/<dev>/microphone\_up](abi-testing.html#abi-sys-bus-hid-drivers-hid-corsair-void-dev-microphone-up)
* [/sys/bus/hid/drivers/hid-corsair-void/<dev>/send\_alert](abi-testing.html#abi-sys-bus-hid-drivers-hid-corsair-void-dev-send-alert)
* [/sys/bus/hid/drivers/hid-corsair-void/<dev>/set\_sidetone](abi-testing.html#abi-sys-bus-hid-drivers-hid-corsair-void-dev-set-sidetone)
* [/sys/bus/hid/drivers/hid-corsair-void/<dev>/sidetone\_max](abi-testing.html#abi-sys-bus-hid-drivers-hid-corsair-void-dev-sidetone-max)

## ABI file testing/sysfs-driver-hid-lenovo

Has the following ABI:

* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/press\_to\_select](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-press-to-select)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/dragging](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-dragging)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/release\_to\_select](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-release-to-select)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/select\_right](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-select-right)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/sensitivity](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-sensitivity)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/press\_speed](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-press-speed)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/fn\_lock](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-fn-lock)

## ABI file testing/sysfs-driver-hid-lenovo-go

Has the following ABI:

* [/sys/class/leds/go:rgb:joystick\_rings/effect](abi-testing.html#abi-sys-class-leds-go-rgb-joystick-rings-effect)
* [/sys/class/leds/go:rgb:joystick\_rings/effect\_index](abi-testing.html#abi-sys-class-leds-go-rgb-joystick-rings-effect-index)
* [/sys/class/leds/go:rgb:joystick\_rings/enabled](abi-testing.html#abi-sys-class-leds-go-rgb-joystick-rings-enabled)
* [/sys/class/leds/go:rgb:joystick\_rings/enabled\_index](abi-testing.html#abi-sys-class-leds-go-rgb-joystick-rings-enabled-index)
* [/sys/class/leds/go:rgb:joystick\_rings/mode](abi-testing.html#abi-sys-class-leds-go-rgb-joystick-rings-mode)
* [/sys/class/leds/go:rgb:joystick\_rings/mode\_index](abi-testing.html#abi-sys-class-leds-go-rgb-joystick-rings-mode-index)
* [/sys/class/leds/go:rgb:joystick\_rings/profile](abi-testing.html#abi-sys-class-leds-go-rgb-joystick-rings-profile)
* [/sys/class/leds/go:rgb:joystick\_rings/profile\_range](abi-testing.html#abi-sys-class-leds-go-rgb-joystick-rings-profile-range)
* [/sys/class/leds/go:rgb:joystick\_rings/speed](abi-testing.html#abi-sys-class-leds-go-rgb-joystick-rings-speed)
* [/sys/class/leds/go:rgb:joystick\_rings/speed\_range](abi-testing.html#abi-sys-class-leds-go-rgb-joystick-rings-speed-range)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/firmware\_version](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-firmware-version)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/fps\_mode\_dpi](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-fps-mode-dpi)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/fps\_mode\_dpi\_index](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-fps-mode-dpi-index)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/hardware\_generation](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-hardware-generation)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/hardware\_version](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-hardware-version)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/left\_handle/auto\_sleep\_time](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-left-handle-auto-sleep-time)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/left\_handle/auto\_sleep\_time\_range](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-left-handle-auto-sleep-time-range)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/left\_handle/calibrate\_gyro](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-left-handle-calibrate-gyro)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/left\_handle/calibrate\_gyro\_index](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-left-handle-calibrate-gyro-index)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/left\_handle/calibrate\_gyro\_status](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-left-handle-calibrate-gyro-status)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/left\_handle/calibrate\_joystick](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-left-handle-calibrate-joystick)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/left\_handle/calibrate\_joystick\_index](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-left-handle-calibrate-joystick-index)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/left\_handle/calibrate\_joystick\_status](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-left-handle-calibrate-joystick-status)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/left\_handle/calibrate\_tirgger](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-left-handle-calibrate-tirgger)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/left\_handle/calibrate\_gyro\_trigger](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-left-handle-calibrate-gyro-trigger)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/left\_handle/calibrate\_trigger\_status](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-left-handle-calibrate-trigger-status)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/left\_handle/firmware\_version](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-left-handle-firmware-version)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/left\_handle/hardware\_generation](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-left-handle-hardware-generation)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/left\_handle/hardware\_version](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-left-handle-hardware-version)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/left\_handle/imu\_bypass\_enabled](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-left-handle-imu-bypass-enabled)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/left\_handle/imu\_bypass\_enabled\_index](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-left-handle-imu-bypass-enabled-index)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/left\_handle/imu\_enabled](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-left-handle-imu-enabled)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/left\_handle/imu\_enabled\_index](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-left-handle-imu-enabled-index)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/left\_handle/product\_version](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-left-handle-product-version)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/left\_handle/protocol\_version](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-left-handle-protocol-version)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/left\_handle/reset](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-left-handle-reset)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/left\_handle/rumble\_mode](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-left-handle-rumble-mode)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/left\_handle/rumble\_mode\_index](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-left-handle-rumble-mode-index)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/left\_handle/rumble\_notification](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-left-handle-rumble-notification)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/left\_handle/rumble\_notification\_index](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-left-handle-rumble-notification-index)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/mode](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-mode)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/left\_handle/mode\_index](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-left-handle-mode-index)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/os\_mode](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-os-mode)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/os\_mode\_index](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-os-mode-index)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/product\_version](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-product-version)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/right\_handle/protocol\_version](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-right-handle-protocol-version)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/reset\_mcu](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-reset-mcu)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/right\_handle/auto\_sleep\_time](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-right-handle-auto-sleep-time)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/right\_handle/auto\_sleep\_time\_range](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-right-handle-auto-sleep-time-range)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/right\_handle/calibrate\_gyro](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-right-handle-calibrate-gyro)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/right\_handle/calibrate\_gyro\_index](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-right-handle-calibrate-gyro-index)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/right\_handle/calibrate\_gyro\_status](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-right-handle-calibrate-gyro-status)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/right\_handle/calibrate\_joystick](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-right-handle-calibrate-joystick)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/right\_handle/calibrate\_joystick\_index](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-right-handle-calibrate-joystick-index)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/right\_handle/calibrate\_joystick\_status](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-right-handle-calibrate-joystick-status)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/right\_handle/calibrate\_tirgger](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-right-handle-calibrate-tirgger)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/right\_handle/calibrate\_gyro\_trigger](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-right-handle-calibrate-gyro-trigger)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/right\_handle/calibrate\_trigger\_status](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-right-handle-calibrate-trigger-status)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/right\_handle/firmware\_version](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-right-handle-firmware-version)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/right\_handle/hardware\_generation](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-right-handle-hardware-generation)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/right\_handle/hardware\_version](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-right-handle-hardware-version)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/right\_handle/imu\_bypass\_enabled](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-right-handle-imu-bypass-enabled)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/right\_handle/imu\_bypass\_enabled\_index](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-right-handle-imu-bypass-enabled-index)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/right\_handle/imu\_enabled](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-right-handle-imu-enabled)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/right\_handle/imu\_enabled\_index](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-right-handle-imu-enabled-index)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/right\_handle/product\_version](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-right-handle-product-version)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/right\_handle/protocol\_version](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-right-handle-protocol-versiono)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/right\_handle/reset](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-right-handle-reset)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/right\_handle/rumble\_mode](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-right-handle-rumble-mode)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/right\_handle/rumble\_mode\_index](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-right-handle-rumble-mode-index)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/right\_handle/rumble\_notification](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-right-handle-rumble-notification)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/right\_handle/rumble\_notification\_index](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-right-handle-rumble-notification-index)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/rumble\_intensity](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-rumble-intensity)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/rumble\_intensity\_index](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-rumble-intensity-index)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/touchpad/enabled](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-touchpad-enabled)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/touchpad/enabled\_index](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-touchpad-enabled-index)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/touchpad/vibration\_enabled](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-touchpad-vibration-enabled)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/touchpad/vibration\_enabled\_index](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-touchpad-vibration-enabled-index)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/touchpad/vibration\_intensity](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-touchpad-vibration-intensity)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/touchpad/vibration\_intensity\_index](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-touchpad-vibration-intensity-index)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/tx\_dongle/firmware\_version](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-tx-dongle-firmware-version)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/tx\_dongle/hardware\_generation](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-tx-dongle-hardware-generation)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/tx\_dongle/hardware\_version](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-tx-dongle-hardware-version)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/tx\_dongle/product\_version](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-tx-dongle-product-version)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/tx\_dongle/protocol\_version](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-tx-dongle-protocol-version)

## ABI file testing/sysfs-driver-hid-lenovo-go-s

Has the following ABI:

* [/sys/class/leds/go\_s:rgb:joystick\_rings/effect](abi-testing.html#abi-sys-class-leds-go-s-rgb-joystick-rings-effect)
* [/sys/class/leds/go\_s:rgb:joystick\_rings/effect\_index](abi-testing.html#abi-sys-class-leds-go-s-rgb-joystick-rings-effect-index)
* [/sys/class/leds/go\_s:rgb:joystick\_rings/enabled](abi-testing.html#abi-sys-class-leds-go-s-rgb-joystick-rings-enabled)
* [/sys/class/leds/go\_s:rgb:joystick\_rings/enabled\_index](abi-testing.html#abi-sys-class-leds-go-s-rgb-joystick-rings-enabled-index)
* [/sys/class/leds/go\_s:rgb:joystick\_rings/mode](abi-testing.html#abi-sys-class-leds-go-s-rgb-joystick-rings-mode)
* [/sys/class/leds/go\_s:rgb:joystick\_rings/mode\_index](abi-testing.html#abi-sys-class-leds-go-s-rgb-joystick-rings-mode-index)
* [/sys/class/leds/go\_s:rgb:joystick\_rings/profile](abi-testing.html#abi-sys-class-leds-go-s-rgb-joystick-rings-profile)
* [/sys/class/leds/go\_s:rgb:joystick\_rings/profile\_range](abi-testing.html#abi-sys-class-leds-go-s-rgb-joystick-rings-profile-range)
* [/sys/class/leds/go\_s:rgb:joystick\_rings/speed](abi-testing.html#abi-sys-class-leds-go-s-rgb-joystick-rings-speed)
* [/sys/class/leds/go\_s:rgb:joystick\_rings/speed\_range](abi-testing.html#abi-sys-class-leds-go-s-rgb-joystick-rings-speed-range)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/gamepad/auto\_sleep\_time](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-gamepad-auto-sleep-time)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/gamepad/auto\_sleep\_time\_range](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-gamepad-auto-sleep-time-range)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/gamepad/dpad\_mode](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-gamepad-dpad-mode)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/gamepad/dpad\_mode\_index](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-gamepad-dpad-mode-index)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/gamepad/mode](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-gamepad-mode)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/gamepad/mode\_index](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-gamepad-mode-index)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/gamepad/poll\_rate](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-gamepad-poll-rate)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/gamepad/poll\_rate\_index](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-gamepad-poll-rate-index)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/imu/bypass\_enabled](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-imu-bypass-enabled)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/imu/bypass\_enabled\_index](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-imu-bypass-enabled-index)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/imu/manufacturer](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-imu-manufacturer)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/imu/sensor\_enabled](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-imu-sensor-enabled)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/imu/sensor\_enabled\_index](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-imu-sensor-enabled-index)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/mcu\_id](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-mcu-id)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/mouse/step](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-mouse-step)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/mouse/step\_range](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-mouse-step-range)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/os\_mode](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-os-modeo)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/os\_mode\_index](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-os-mode-indexo)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/touchpad/enabled](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-touchpad-enabledo)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/touchpad/enabled\_index](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-touchpad-enabled-indexo)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/touchpad/linux\_mode](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-touchpad-linux-mode)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/touchpad/linux\_mode\_index](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-touchpad-linux-mode-index)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/touchpad/windows\_mode](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-touchpad-windows-mode)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/touchpad/windows\_mode\_index](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-touchpad-windows-mode-index)

## ABI file testing/sysfs-driver-hid-logitech-hidpp

Has the following ABI:

* [/sys/bus/hid/drivers/logitech-hidpp-device/<dev>/range](abi-testing.html#abi-sys-bus-hid-drivers-logitech-hidpp-device-dev-range)
* [/sys/bus/hid/drivers/logitech-hidpp-device/<dev>/builtin\_power\_supply](abi-testing.html#abi-sys-bus-hid-drivers-logitech-hidpp-device-dev-builtin-power-supply)

## ABI file testing/sysfs-driver-hid-logitech-lg4ff

Has the following ABI:

* [/sys/bus/hid/drivers/logitech/<dev>/range](abi-testing.html#abi-sys-bus-hid-drivers-logitech-dev-range)
* [/sys/bus/hid/drivers/logitech/<dev>/alternate\_modes](abi-testing.html#abi-sys-bus-hid-drivers-logitech-dev-alternate-modes)
* [/sys/bus/hid/drivers/logitech/<dev>/real\_id](abi-testing.html#abi-sys-bus-hid-drivers-logitech-dev-real-id)
* [/sys/bus/hid/drivers/logitech/<dev>/combine\_pedals](abi-testing.html#abi-sys-bus-hid-drivers-logitech-dev-combine-pedals)

## ABI file testing/sysfs-driver-hid-multitouch

Has the following ABI:

* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/quirks](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-quirks)

## ABI file testing/sysfs-driver-hid-ntrig

Has the following ABI:

* [/sys/bus/hid/drivers/ntrig/<dev>/activate\_slack](abi-testing.html#abi-sys-bus-hid-drivers-ntrig-dev-activate-slack)
* [/sys/bus/hid/drivers/ntrig/<dev>/decativate\_slack](abi-testing.html#abi-sys-bus-hid-drivers-ntrig-dev-decativate-slack)
* [/sys/bus/hid/drivers/ntrig/<dev>/activation\_width](abi-testing.html#abi-sys-bus-hid-drivers-ntrig-dev-activation-width)
* [/sys/bus/hid/drivers/ntrig/<dev>/activation\_height](abi-testing.html#abi-sys-bus-hid-drivers-ntrig-dev-activation-width)
* [/sys/bus/hid/drivers/ntrig/<dev>/min\_width](abi-testing.html#abi-sys-bus-hid-drivers-ntrig-dev-min-width)
* [/sys/bus/hid/drivers/ntrig/<dev>/min\_height](abi-testing.html#abi-sys-bus-hid-drivers-ntrig-dev-min-width)
* [/sys/bus/hid/drivers/ntrig/<dev>/sensor\_physical\_width](abi-testing.html#abi-sys-bus-hid-drivers-ntrig-dev-sensor-physical-width)
* [/sys/bus/hid/drivers/ntrig/<dev>/sensor\_physical\_height](abi-testing.html#abi-sys-bus-hid-drivers-ntrig-dev-sensor-physical-width)
* [/sys/bus/hid/drivers/ntrig/<dev>/sensor\_logical\_width](abi-testing.html#abi-sys-bus-hid-drivers-ntrig-dev-sensor-logical-width)
* [/sys/bus/hid/drivers/ntrig/<dev>/sensor\_logical\_height](abi-testing.html#abi-sys-bus-hid-drivers-ntrig-dev-sensor-logical-width)

## ABI file testing/sysfs-driver-hid-picolcd

Has the following ABI:

* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/operation\_mode](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-operation-mode)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/operation\_mode\_delay](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-operation-mode-delay)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/fb\_update\_rate](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-fb-update-rate)

## ABI file testing/sysfs-driver-hid-prodikeys

Has the following ABI:

* [/sys/bus/hid/drivers/prodikeys/.../channel](abi-testing.html#abi-sys-bus-hid-drivers-prodikeys-channel)
* [/sys/bus/hid/drivers/prodikeys/.../sustain](abi-testing.html#abi-sys-bus-hid-drivers-prodikeys-sustain)
* [/sys/bus/hid/drivers/prodikeys/.../octave](abi-testing.html#abi-sys-bus-hid-drivers-prodikeys-octave)

## ABI file testing/sysfs-driver-hid-roccat-kone

Has the following ABI:

* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/kone/roccatkone<minor>/actual\_dpi](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-kone-roccatkone-minor-actual-dpi)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/kone/roccatkone<minor>/actual\_profile](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-kone-roccatkone-minor-actual-profile)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/kone/roccatkone<minor>/firmware\_version](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-kone-roccatkone-minor-firmware-version)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/kone/roccatkone<minor>/profile[1-5]](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-kone-roccatkone-minor-profile-1-5)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/kone/roccatkone<minor>/settings](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-kone-roccatkone-minor-settings)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/kone/roccatkone<minor>/startup\_profile](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-kone-roccatkone-minor-startup-profile)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/kone/roccatkone<minor>/tcu](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-kone-roccatkone-minor-tcu)
* [/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/kone/roccatkone<minor>/weight](abi-testing.html#abi-sys-bus-usb-devices-busnum-devnum-config-num-interface-num-hid-bus-vendor-id-product-id-num-kone-roccatkone-minor-weight)

## ABI file testing/sysfs-driver-hid-srws1

Has the following ABI:

* [/sys/class/leds/SRWS1::<serial>::RPM1](abi-testing.html#abi-sys-class-leds-srws1-serial-rpm1)
* [/sys/class/leds/SRWS1::<serial>::RPM2](abi-testing.html#abi-sys-class-leds-srws1-serial-rpm1)
* [/sys/class/leds/SRWS1::<serial>::RPM3](abi-testing.html#abi-sys-class-leds-srws1-serial-rpm1)
* [/sys/class/leds/SRWS1::<serial>::RPM4](abi-testing.html#abi-sys-class-leds-srws1-serial-rpm1)
* [/sys/class/leds/SRWS1::<serial>::RPM5](abi-testing.html#abi-sys-class-leds-srws1-serial-rpm1)
* [/sys/class/leds/SRWS1::<serial>::RPM6](abi-testing.html#abi-sys-class-leds-srws1-serial-rpm1)
* [/sys/class/leds/SRWS1::<serial>::RPM7](abi-testing.html#abi-sys-class-leds-srws1-serial-rpm1)
* [/sys/class/leds/SRWS1::<serial>::RPM8](abi-testing.html#abi-sys-class-leds-srws1-serial-rpm1)
* [/sys/class/leds/SRWS1::<serial>::RPM9](abi-testing.html#abi-sys-class-leds-srws1-serial-rpm1)
* [/sys/class/leds/SRWS1::<serial>::RPM10](abi-testing.html#abi-sys-class-leds-srws1-serial-rpm1)
* [/sys/class/leds/SRWS1::<serial>::RPM11](abi-testing.html#abi-sys-class-leds-srws1-serial-rpm1)
* [/sys/class/leds/SRWS1::<serial>::RPM12](abi-testing.html#abi-sys-class-leds-srws1-serial-rpm1)
* [/sys/class/leds/SRWS1::<serial>::RPM13](abi-testing.html#abi-sys-class-leds-srws1-serial-rpm1)
* [/sys/class/leds/SRWS1::<serial>::RPM14](abi-testing.html#abi-sys-class-leds-srws1-serial-rpm1)
* [/sys/class/leds/SRWS1::<serial>::RPM15](abi-testing.html#abi-sys-class-leds-srws1-serial-rpm1)
* [/sys/class/leds/SRWS1::<serial>::RPMALL](abi-testing.html#abi-sys-class-leds-srws1-serial-rpm1)

## ABI file testing/sysfs-driver-hid-wiimote

Has the following ABI:

* [/sys/bus/hid/drivers/wiimote/<dev>/led1](abi-testing.html#abi-sys-bus-hid-drivers-wiimote-dev-led1)
* [/sys/bus/hid/drivers/wiimote/<dev>/led2](abi-testing.html#abi-sys-bus-hid-drivers-wiimote-dev-led1)
* [/sys/bus/hid/drivers/wiimote/<dev>/led3](abi-testing.html#abi-sys-bus-hid-drivers-wiimote-dev-led1)
* [/sys/bus/hid/drivers/wiimote/<dev>/led4](abi-testing.html#abi-sys-bus-hid-drivers-wiimote-dev-led1)
* [/sys/bus/hid/drivers/wiimote/<dev>/extension](abi-testing.html#abi-sys-bus-hid-drivers-wiimote-dev-extension)
* [/sys/bus/hid/drivers/wiimote/<dev>/devtype](abi-testing.html#abi-sys-bus-hid-drivers-wiimote-dev-devtype)
* [/sys/bus/hid/drivers/wiimote/<dev>/bboard\_calib](abi-testing.html#abi-sys-bus-hid-drivers-wiimote-dev-bboard-calib)
* [/sys/bus/hid/drivers/wiimote/<dev>/pro\_calib](abi-testing.html#abi-sys-bus-hid-drivers-wiimote-dev-pro-calib)

## ABI file testing/sysfs-driver-input-axp-pek

Has the following ABI:

* [/sys/class/input/input(x)/device/startup](abi-testing.html#abi-sys-class-input-input-x-device-startup)
* [/sys/class/input/input(x)/device/shutdown](abi-testing.html#abi-sys-class-input-input-x-device-shutdown)

## ABI file testing/sysfs-driver-input-cros-ec-keyb

Has the following ABI:

* [/sys/class/input/input(x)/device/function\_row\_physmap](abi-testing.html#abi-sys-class-input-input-x-device-function-row-physmap)

## ABI file testing/sysfs-driver-input-exc3000

Has the following ABI:

* [/sys/bus/i2c/devices/xxx/fw\_version](abi-testing.html#abi-sys-bus-i2c-devices-xxx-fw-version)
* [/sys/bus/i2c/devices/xxx/model](abi-testing.html#abi-sys-bus-i2c-devices-xxx-model)
* [/sys/bus/i2c/devices/xxx/type](abi-testing.html#abi-sys-bus-i2c-devices-xxx-type)

## ABI file testing/sysfs-driver-intc\_sar

Has the following ABI:

* [/sys/bus/platform/devices/INTC1092:00/intc\_reg](abi-testing.html#abi-sys-bus-platform-devices-intc1092-00-intc-reg)
* [/sys/bus/platform/devices/INTC1092:00/intc\_data](abi-testing.html#abi-sys-bus-platform-devices-intc1092-00-intc-data)

## ABI file testing/sysfs-driver-intel-i915-hwmon

Has the following ABI:

* [/sys/bus/pci/drivers/i915/.../hwmon/hwmon<i>/in0\_input](abi-testing.html#abi-sys-bus-pci-drivers-i915-hwmon-hwmon-i-in0-input)
* [/sys/bus/pci/drivers/i915/.../hwmon/hwmon<i>/power1\_max](abi-testing.html#abi-sys-bus-pci-drivers-i915-hwmon-hwmon-i-power1-max)
* [/sys/bus/pci/drivers/i915/.../hwmon/hwmon<i>/power1\_rated\_max](abi-testing.html#abi-sys-bus-pci-drivers-i915-hwmon-hwmon-i-power1-rated-max)
* [/sys/bus/pci/drivers/i915/.../hwmon/hwmon<i>/power1\_max\_interval](abi-testing.html#abi-sys-bus-pci-drivers-i915-hwmon-hwmon-i-power1-max-interval)
* [/sys/bus/pci/drivers/i915/.../hwmon/hwmon<i>/power1\_crit](abi-testing.html#abi-sys-bus-pci-drivers-i915-hwmon-hwmon-i-power1-crit)
* [/sys/bus/pci/drivers/i915/.../hwmon/hwmon<i>/curr1\_crit](abi-testing.html#abi-sys-bus-pci-drivers-i915-hwmon-hwmon-i-curr1-crit)
* [/sys/bus/pci/drivers/i915/.../hwmon/hwmon<i>/energy1\_input](abi-testing.html#abi-sys-bus-pci-drivers-i915-hwmon-hwmon-i-energy1-input)
* [/sys/bus/pci/drivers/i915/.../hwmon/hwmon<i>/fan1\_input](abi-testing.html#abi-sys-bus-pci-drivers-i915-hwmon-hwmon-i-fan1-input)
* [/sys/bus/pci/drivers/i915/.../hwmon/hwmon<i>/temp1\_input](abi-testing.html#abi-sys-bus-pci-drivers-i915-hwmon-hwmon-i-temp1-input)

## ABI file testing/sysfs-driver-intel-m10-bmc

Has the following ABI:

* [/sys/bus/.../drivers/intel-m10-bmc/.../bmc\_version](abi-testing.html#abi-sys-bus-drivers-intel-m10-bmc-bmc-version)
* [/sys/bus/.../drivers/intel-m10-bmc/.../bmcfw\_version](abi-testing.html#abi-sys-bus-drivers-intel-m10-bmc-bmcfw-version)
* [/sys/bus/.../drivers/intel-m10-bmc/.../mac\_address](abi-testing.html#abi-sys-bus-drivers-intel-m10-bmc-mac-address)
* [/sys/bus/.../drivers/intel-m10-bmc/.../mac\_count](abi-testing.html#abi-sys-bus-drivers-intel-m10-bmc-mac-count)

## ABI file testing/sysfs-driver-intel-m10-bmc-sec-update

Has the following ABI:

* [/sys/bus/platform/drivers/intel-m10bmc-sec-update/.../security/sr\_root\_entry\_hash](abi-testing.html#abi-sys-bus-platform-drivers-intel-m10bmc-sec-update-security-sr-root-entry-hash)
* [/sys/bus/platform/drivers/intel-m10bmc-sec-update/.../security/pr\_root\_entry\_hash](abi-testing.html#abi-sys-bus-platform-drivers-intel-m10bmc-sec-update-security-pr-root-entry-hash)
* [/sys/bus/platform/drivers/intel-m10bmc-sec-update/.../security/bmc\_root\_entry\_hash](abi-testing.html#abi-sys-bus-platform-drivers-intel-m10bmc-sec-update-security-bmc-root-entry-hash)
* [/sys/bus/platform/drivers/intel-m10bmc-sec-update/.../security/sr\_canceled\_csks](abi-testing.html#abi-sys-bus-platform-drivers-intel-m10bmc-sec-update-security-sr-canceled-csks)
* [/sys/bus/platform/drivers/intel-m10bmc-sec-update/.../security/pr\_canceled\_csks](abi-testing.html#abi-sys-bus-platform-drivers-intel-m10bmc-sec-update-security-pr-canceled-csks)
* [/sys/bus/platform/drivers/intel-m10bmc-sec-update/.../security/bmc\_canceled\_csks](abi-testing.html#abi-sys-bus-platform-drivers-intel-m10bmc-sec-update-security-bmc-canceled-csks)
* [/sys/bus/platform/drivers/intel-m10bmc-sec-update/.../security/flash\_count](abi-testing.html#abi-sys-bus-platform-drivers-intel-m10bmc-sec-update-security-flash-count)

## ABI file testing/sysfs-driver-intel-rapid-start

Has the following ABI:

* [/sys/bus/acpi/intel-rapid-start/wakeup\_events](abi-testing.html#abi-sys-bus-acpi-intel-rapid-start-wakeup-events)
* [/sys/bus/acpi/intel-rapid-start/wakeup\_time](abi-testing.html#abi-sys-bus-acpi-intel-rapid-start-wakeup-time)

## ABI file testing/sysfs-driver-intel-xe-hwmon

Has the following ABI:

* [/sys/bus/pci/drivers/xe/.../hwmon/hwmon<i>/power1\_max](abi-testing.html#abi-sys-bus-pci-drivers-xe-hwmon-hwmon-i-power1-max)
* [/sys/bus/pci/drivers/xe/.../hwmon/hwmon<i>/power1\_rated\_max](abi-testing.html#abi-sys-bus-pci-drivers-xe-hwmon-hwmon-i-power1-rated-max)
* [/sys/bus/pci/drivers/xe/.../hwmon/hwmon<i>/energy1\_input](abi-testing.html#abi-sys-bus-pci-drivers-xe-hwmon-hwmon-i-energy1-input)
* [/sys/bus/pci/drivers/xe/.../hwmon/hwmon<i>/power1\_max\_interval](abi-testing.html#abi-sys-bus-pci-drivers-xe-hwmon-hwmon-i-power1-max-interval)
* [/sys/bus/pci/drivers/xe/.../hwmon/hwmon<i>/power2\_max](abi-testing.html#abi-sys-bus-pci-drivers-xe-hwmon-hwmon-i-power2-max)
* [/sys/bus/pci/drivers/xe/.../hwmon/hwmon<i>/power2\_rated\_max](abi-testing.html#abi-sys-bus-pci-drivers-xe-hwmon-hwmon-i-power2-rated-max)
* [/sys/bus/pci/drivers/xe/.../hwmon/hwmon<i>/power1\_crit](abi-testing.html#abi-sys-bus-pci-drivers-xe-hwmon-hwmon-i-power1-crit)
* [/sys/bus/pci/drivers/xe/.../hwmon/hwmon<i>/curr1\_crit](abi-testing.html#abi-sys-bus-pci-drivers-xe-hwmon-hwmon-i-curr1-crit)
* [/sys/bus/pci/drivers/xe/.../hwmon/hwmon<i>/energy2\_input](abi-testing.html#abi-sys-bus-pci-drivers-xe-hwmon-hwmon-i-energy2-input)
* [/sys/bus/pci/drivers/xe/.../hwmon/hwmon<i>/power2\_max\_interval](abi-testing.html#abi-sys-bus-pci-drivers-xe-hwmon-hwmon-i-power2-max-interval)
* [/sys/bus/pci/drivers/xe/.../hwmon/hwmon<i>/in1\_input](abi-testing.html#abi-sys-bus-pci-drivers-xe-hwmon-hwmon-i-in1-input)
* [/sys/bus/pci/drivers/xe/.../hwmon/hwmon<i>/temp2\_crit](abi-testing.html#abi-sys-bus-pci-drivers-xe-hwmon-hwmon-i-temp2-crit)
* [/sys/bus/pci/drivers/xe/.../hwmon/hwmon<i>/temp2\_emergency](abi-testing.html#abi-sys-bus-pci-drivers-xe-hwmon-hwmon-i-temp2-emergency)
* [/sys/bus/pci/drivers/xe/.../hwmon/hwmon<i>/temp2\_input](abi-testing.html#abi-sys-bus-pci-drivers-xe-hwmon-hwmon-i-temp2-input)
* [/sys/bus/pci/drivers/xe/.../hwmon/hwmon<i>/temp2\_max](abi-testing.html#abi-sys-bus-pci-drivers-xe-hwmon-hwmon-i-temp2-max)
* [/sys/bus/pci/drivers/xe/.../hwmon/hwmon<i>/temp3\_crit](abi-testing.html#abi-sys-bus-pci-drivers-xe-hwmon-hwmon-i-temp3-crit)
* [/sys/bus/pci/drivers/xe/.../hwmon/hwmon<i>/temp3\_emergency](abi-testing.html#abi-sys-bus-pci-drivers-xe-hwmon-hwmon-i-temp3-emergency)
* [/sys/bus/pci/drivers/xe/.../hwmon/hwmon<i>/temp3\_input](abi-testing.html#abi-sys-bus-pci-drivers-xe-hwmon-hwmon-i-temp3-input)
* [/sys/bus/pci/drivers/xe/.../hwmon/hwmon<i>/temp4\_crit](abi-testing.html#abi-sys-bus-pci-drivers-xe-hwmon-hwmon-i-temp4-crit)
* [/sys/bus/pci/drivers/xe/.../hwmon/hwmon<i>/temp4\_emergency](abi-testing.html#abi-sys-bus-pci-drivers-xe-hwmon-hwmon-i-temp4-emergency)
* [/sys/bus/pci/drivers/xe/.../hwmon/hwmon<i>/temp4\_input](abi-testing.html#abi-sys-bus-pci-drivers-xe-hwmon-hwmon-i-temp4-input)
* [/sys/bus/pci/drivers/xe/.../hwmon/hwmon<i>/temp5\_crit](abi-testing.html#abi-sys-bus-pci-drivers-xe-hwmon-hwmon-i-temp5-crit)
* [/sys/bus/pci/drivers/xe/.../hwmon/hwmon<i>/temp5\_emergency](abi-testing.html#abi-sys-bus-pci-drivers-xe-hwmon-hwmon-i-temp5-emergency)
* [/sys/bus/pci/drivers/xe/.../hwmon/hwmon<i>/temp5\_input](abi-testing.html#abi-sys-bus-pci-drivers-xe-hwmon-hwmon-i-temp5-input)
* [/sys/bus/pci/drivers/xe/.../hwmon/hwmon<i>/temp[6-21]\_crit](abi-testing.html#abi-sys-bus-pci-drivers-xe-hwmon-hwmon-i-temp-6-21-crit)
* [/sys/bus/pci/drivers/xe/.../hwmon/hwmon<i>/temp[6-21]\_emergency](abi-testing.html#abi-sys-bus-pci-drivers-xe-hwmon-hwmon-i-temp-6-21-emergency)
* [/sys/bus/pci/drivers/xe/.../hwmon/hwmon<i>/temp[6-21]\_input](abi-testing.html#abi-sys-bus-pci-drivers-xe-hwmon-hwmon-i-temp-6-21-input)
* [/sys/bus/pci/drivers/xe/.../hwmon/hwmon<i>/fan1\_input](abi-testing.html#abi-sys-bus-pci-drivers-xe-hwmon-hwmon-i-fan1-input)
* [/sys/bus/pci/drivers/xe/.../hwmon/hwmon<i>/fan2\_input](abi-testing.html#abi-sys-bus-pci-drivers-xe-hwmon-hwmon-i-fan2-input)
* [/sys/bus/pci/drivers/xe/.../hwmon/hwmon<i>/fan3\_input](abi-testing.html#abi-sys-bus-pci-drivers-xe-hwmon-hwmon-i-fan3-input)
* [/sys/bus/pci/drivers/xe/.../hwmon/hwmon<i>/power1\_cap](abi-testing.html#abi-sys-bus-pci-drivers-xe-hwmon-hwmon-i-power1-cap)
* [/sys/bus/pci/drivers/xe/.../hwmon/hwmon<i>/power2\_cap](abi-testing.html#abi-sys-bus-pci-drivers-xe-hwmon-hwmon-i-power2-cap)
* [/sys/bus/pci/drivers/xe/.../hwmon/hwmon<i>/power1\_cap\_interval](abi-testing.html#abi-sys-bus-pci-drivers-xe-hwmon-hwmon-i-power1-cap-interval)
* [/sys/bus/pci/drivers/xe/.../hwmon/hwmon<i>/power2\_cap\_interval](abi-testing.html#abi-sys-bus-pci-drivers-xe-hwmon-hwmon-i-power2-cap-interval)

## ABI file testing/sysfs-driver-intel-xe-sriov

Has the following ABI:

* [/sys/bus/pci/drivers/xe/.../sriov\_admin/](abi-testing.html#abi-sys-bus-pci-drivers-xe-sriov-admin)
* [/sys/bus/pci/drivers/xe/.../sriov\_admin/pf/](abi-testing.html#abi-sys-bus-pci-drivers-xe-sriov-admin-pf)
* [/sys/bus/pci/drivers/xe/.../sriov\_admin/vf1/](abi-testing.html#abi-sys-bus-pci-drivers-xe-sriov-admin-vf1)
* [/sys/bus/pci/drivers/xe/.../sriov\_admin/vf2/](abi-testing.html#abi-sys-bus-pci-drivers-xe-sriov-admin-vf1)
* [/sys/bus/pci/drivers/xe/.../sriov\_admin/vf<N>/](abi-testing.html#abi-sys-bus-pci-drivers-xe-sriov-admin-vf1)
* [/sys/bus/pci/drivers/xe/.../sriov\_admin/pf/profile/exec\_quantum\_ms](abi-testing.html#abi-sys-bus-pci-drivers-xe-sriov-admin-pf-profile-exec-quantum-ms)
* [/sys/bus/pci/drivers/xe/.../sriov\_admin/pf/profile/preempt\_timeout\_us](abi-testing.html#abi-sys-bus-pci-drivers-xe-sriov-admin-pf-profile-exec-quantum-ms)
* [/sys/bus/pci/drivers/xe/.../sriov\_admin/pf/profile/sched\_priority](abi-testing.html#abi-sys-bus-pci-drivers-xe-sriov-admin-pf-profile-exec-quantum-ms)
* [/sys/bus/pci/drivers/xe/.../sriov\_admin/vf<n>/profile/exec\_quantum\_ms](abi-testing.html#abi-sys-bus-pci-drivers-xe-sriov-admin-pf-profile-exec-quantum-ms)
* [/sys/bus/pci/drivers/xe/.../sriov\_admin/vf<n>/profile/preempt\_timeout\_us](abi-testing.html#abi-sys-bus-pci-drivers-xe-sriov-admin-pf-profile-exec-quantum-ms)
* [/sys/bus/pci/drivers/xe/.../sriov\_admin/vf<n>/profile/sched\_priority](abi-testing.html#abi-sys-bus-pci-drivers-xe-sriov-admin-pf-profile-exec-quantum-ms)
* [/sys/bus/pci/drivers/xe/.../sriov\_admin/.bulk\_profile/exec\_quantum\_ms](abi-testing.html#abi-sys-bus-pci-drivers-xe-sriov-admin-bulk-profile-exec-quantum-ms)
* [/sys/bus/pci/drivers/xe/.../sriov\_admin/.bulk\_profile/preempt\_timeout\_us](abi-testing.html#abi-sys-bus-pci-drivers-xe-sriov-admin-bulk-profile-exec-quantum-ms)
* [/sys/bus/pci/drivers/xe/.../sriov\_admin/.bulk\_profile/sched\_priority](abi-testing.html#abi-sys-bus-pci-drivers-xe-sriov-admin-bulk-profile-exec-quantum-ms)
* [/sys/bus/pci/drivers/xe/.../sriov\_admin/.bulk\_profile/vram\_quota](abi-testing.html#abi-sys-bus-pci-drivers-xe-sriov-admin-bulk-profile-vram-quota)
* [/sys/bus/pci/drivers/xe/.../sriov\_admin/vf<n>/profile/vram\_quota](abi-testing.html#abi-sys-bus-pci-drivers-xe-sriov-admin-bulk-profile-vram-quota)
* [/sys/bus/pci/drivers/xe/.../sriov\_admin/vf<n>/stop](abi-testing.html#abi-sys-bus-pci-drivers-xe-sriov-admin-vf-n-stop)
* [/sys/bus/pci/drivers/xe/.../sriov\_admin/pf/device](abi-testing.html#abi-sys-bus-pci-drivers-xe-sriov-admin-pf-device)
* [/sys/bus/pci/drivers/xe/.../sriov\_admin/vf<n>/device](abi-testing.html#abi-sys-bus-pci-drivers-xe-sriov-admin-pf-device)

## ABI file testing/sysfs-driver-intel\_sdsi

Has the following ABI:

* [/sys/bus/auxiliary/devices/intel\_vsec.sdsi.X](abi-testing.html#abi-sys-bus-auxiliary-devices-intel-vsec-sdsi-x)
* [/sys/bus/auxiliary/devices/intel\_vsec.sdsi.X/guid](abi-testing.html#abi-sys-bus-auxiliary-devices-intel-vsec-sdsi-x-guid)
* [/sys/bus/auxiliary/devices/intel\_vsec.sdsi.X/registers](abi-testing.html#abi-sys-bus-auxiliary-devices-intel-vsec-sdsi-x-registers)
* [/sys/bus/auxiliary/devices/intel\_vsec.sdsi.X/provision\_akc](abi-testing.html#abi-sys-bus-auxiliary-devices-intel-vsec-sdsi-x-provision-akc)
* [/sys/bus/auxiliary/devices/intel\_vsec.sdsi.X/provision\_cap](abi-testing.html#abi-sys-bus-auxiliary-devices-intel-vsec-sdsi-x-provision-cap)
* [/sys/bus/auxiliary/devices/intel\_vsec.sdsi.X/meter\_certificate](abi-testing.html#abi-sys-bus-auxiliary-devices-intel-vsec-sdsi-x-meter-certificate)
* [/sys/bus/auxiliary/devices/intel\_vsec.sdsi.X/state\_certificate](abi-testing.html#abi-sys-bus-auxiliary-devices-intel-vsec-sdsi-x-state-certificate)

## ABI file testing/sysfs-driver-jz4780-efuse

Has the following ABI:

* [/sys/devices/\*/<our-device>/nvmem](abi-testing.html#abi-sys-devices-our-device-nvmem)

## ABI file testing/sysfs-driver-panfrost-profiling

Has the following ABI:

* [/sys/bus/platform/drivers/panfrost/.../profiling](abi-testing.html#abi-sys-bus-platform-drivers-panfrost-profiling)

## ABI file testing/sysfs-driver-panthor-profiling

Has the following ABI:

* [/sys/bus/platform/drivers/panthor/.../profiling](abi-testing.html#abi-sys-bus-platform-drivers-panthor-profiling)

## ABI file testing/sysfs-driver-pciback

Has the following ABI:

* [/sys/bus/pci/drivers/pciback/quirks](abi-testing.html#abi-sys-bus-pci-drivers-pciback-quirks)
* [/sys/bus/pci/drivers/pciback/allow\_interrupt\_control](abi-testing.html#abi-sys-bus-pci-drivers-pciback-allow-interrupt-control)

## ABI file testing/sysfs-driver-ppi

Has the following ABI:

* [/sys/class/tpm/tpmX/ppi/](abi-testing.html#abi-sys-class-tpm-tpmx-ppi)
* [/sys/class/tpm/tpmX/ppi/version](abi-testing.html#abi-sys-class-tpm-tpmx-ppi-version)
* [/sys/class/tpm/tpmX/ppi/request](abi-testing.html#abi-sys-class-tpm-tpmx-ppi-request)
* [/sys/class/tpm/tpmX/ppi/response](abi-testing.html#abi-sys-class-tpm-tpmx-ppi-response)
* [/sys/class/tpm/tpmX/ppi/transition\_action](abi-testing.html#abi-sys-class-tpm-tpmx-ppi-transition-action)
* [/sys/class/tpm/tpmX/ppi/tcg\_operations](abi-testing.html#abi-sys-class-tpm-tpmx-ppi-tcg-operations)
* [/sys/class/tpm/tpmX/ppi/vs\_operations](abi-testing.html#abi-sys-class-tpm-tpmx-ppi-vs-operations)

## ABI file testing/sysfs-driver-qaic

Has the following ABI:

* [/sys/bus/pci/drivers/qaic/XXXX:XX:XX.X/ce\_count](abi-testing.html#abi-sys-bus-pci-drivers-qaic-xxxx-xx-xx-x-ce-count)
* [/sys/bus/pci/drivers/qaic/XXXX:XX:XX.X/ue\_count](abi-testing.html#abi-sys-bus-pci-drivers-qaic-xxxx-xx-xx-x-ue-count)
* [/sys/bus/pci/drivers/qaic/XXXX:XX:XX.X/ue\_nonfatal\_count](abi-testing.html#abi-sys-bus-pci-drivers-qaic-xxxx-xx-xx-x-ue-nonfatal-count)

## ABI file testing/sysfs-driver-qat

Has the following ABI:

* [/sys/bus/pci/devices/<BDF>/qat/state](abi-testing.html#abi-sys-bus-pci-devices-bdf-qat-state)
* [/sys/bus/pci/devices/<BDF>/qat/cfg\_services](abi-testing.html#abi-sys-bus-pci-devices-bdf-qat-cfg-services)
* [/sys/bus/pci/devices/<BDF>/qat/pm\_idle\_enabled](abi-testing.html#abi-sys-bus-pci-devices-bdf-qat-pm-idle-enabled)
* [/sys/bus/pci/devices/<BDF>/qat/rp2srv](abi-testing.html#abi-sys-bus-pci-devices-bdf-qat-rp2srv)
* [/sys/bus/pci/devices/<BDF>/qat/num\_rps](abi-testing.html#abi-sys-bus-pci-devices-bdf-qat-num-rps)
* [/sys/bus/pci/devices/<BDF>/qat/auto\_reset](abi-testing.html#abi-sys-bus-pci-devices-bdf-qat-auto-reset)

## ABI file testing/sysfs-driver-qat\_ras

Has the following ABI:

* [/sys/bus/pci/devices/<BDF>/qat\_ras/errors\_correctable](abi-testing.html#abi-sys-bus-pci-devices-bdf-qat-ras-errors-correctable)
* [/sys/bus/pci/devices/<BDF>/qat\_ras/errors\_nonfatal](abi-testing.html#abi-sys-bus-pci-devices-bdf-qat-ras-errors-nonfatal)
* [/sys/bus/pci/devices/<BDF>/qat\_ras/errors\_fatal](abi-testing.html#abi-sys-bus-pci-devices-bdf-qat-ras-errors-fatal)
* [/sys/bus/pci/devices/<BDF>/qat\_ras/reset\_error\_counters](abi-testing.html#abi-sys-bus-pci-devices-bdf-qat-ras-reset-error-counters)

## ABI file testing/sysfs-driver-qat\_rl

Has the following ABI:

* [/sys/bus/pci/devices/<BDF>/qat\_rl/sla\_op](abi-testing.html#abi-sys-bus-pci-devices-bdf-qat-rl-sla-op)
* [/sys/bus/pci/devices/<BDF>/qat\_rl/rp](abi-testing.html#abi-sys-bus-pci-devices-bdf-qat-rl-rp)
* [/sys/bus/pci/devices/<BDF>/qat\_rl/id](abi-testing.html#abi-sys-bus-pci-devices-bdf-qat-rl-id)
* [/sys/bus/pci/devices/<BDF>/qat\_rl/cir](abi-testing.html#abi-sys-bus-pci-devices-bdf-qat-rl-cir)
* [/sys/bus/pci/devices/<BDF>/qat\_rl/pir](abi-testing.html#abi-sys-bus-pci-devices-bdf-qat-rl-pir)
* [/sys/bus/pci/devices/<BDF>/qat\_rl/srv](abi-testing.html#abi-sys-bus-pci-devices-bdf-qat-rl-srv)
* [/sys/bus/pci/devices/<BDF>/qat\_rl/cap\_rem](abi-testing.html#abi-sys-bus-pci-devices-bdf-qat-rl-cap-rem)

## ABI file testing/sysfs-driver-qat\_svn

Has the following ABI:

* [/sys/bus/pci/devices/<BDF>/qat\_svn/](abi-testing.html#abi-sys-bus-pci-devices-bdf-qat-svn)
* [/sys/bus/pci/devices/<BDF>/qat\_svn/enforced\_min](abi-testing.html#abi-sys-bus-pci-devices-bdf-qat-svn-enforced-min)
* [/sys/bus/pci/devices/<BDF>/qat\_svn/permanent\_min](abi-testing.html#abi-sys-bus-pci-devices-bdf-qat-svn-permanent-min)
* [/sys/bus/pci/devices/<BDF>/qat\_svn/active](abi-testing.html#abi-sys-bus-pci-devices-bdf-qat-svn-active)
* [/sys/bus/pci/devices/<BDF>/qat\_svn/commit](abi-testing.html#abi-sys-bus-pci-devices-bdf-qat-svn-commit)

## ABI file testing/sysfs-driver-samsung-laptop

Has the following ABI:

* [/sys/devices/platform/samsung/performance\_level](abi-testing.html#abi-sys-devices-platform-samsung-performance-level)
* [/sys/devices/platform/samsung/usb\_charge](abi-testing.html#abi-sys-devices-platform-samsung-usb-charge)
* [/sys/devices/platform/samsung/lid\_handling](abi-testing.html#abi-sys-devices-platform-samsung-lid-handling)

## ABI file testing/sysfs-driver-spi-intel

Has the following ABI:

* [/sys/devices/.../intel\_spi\_protected](abi-testing.html#abi-sys-devices-intel-spi-protected)
* [/sys/devices/.../intel\_spi\_locked](abi-testing.html#abi-sys-devices-intel-spi-locked)
* [/sys/devices/.../intel\_spi\_bios\_locked](abi-testing.html#abi-sys-devices-intel-spi-bios-locked)

## ABI file testing/sysfs-driver-st

Has the following ABI:

* [/sys/bus/scsi/drivers/st/debug\_flag](abi-testing.html#abi-sys-bus-scsi-drivers-st-debug-flag)

## ABI file testing/sysfs-driver-tegra-fuse

Has the following ABI:

* [/sys/devices/\*/<our-device>/fuse](abi-testing.html#abi-sys-devices-our-device-fuse)

## ABI file testing/sysfs-driver-toshiba\_acpi

Has the following ABI:

* [/sys/devices/LNXSYSTM:00/LNXSYBUS:00/TOS{1900,620{0,7,8}}:00/kbd\_backlight\_mode](abi-testing.html#abi-sys-devices-lnxsystm-00-lnxsybus-00-tos-1900-620-0-7-8-00-kbd-backlight-mode)
* [/sys/devices/LNXSYSTM:00/LNXSYBUS:00/TOS{1900,620{0,7,8}}:00/kbd\_backlight\_timeout](abi-testing.html#abi-sys-devices-lnxsystm-00-lnxsybus-00-tos-1900-620-0-7-8-00-kbd-backlight-timeout)
* [/sys/devices/LNXSYSTM:00/LNXSYBUS:00/TOS{1900,620{0,7,8}}:00/position](abi-testing.html#abi-sys-devices-lnxsystm-00-lnxsybus-00-tos-1900-620-0-7-8-00-position)
* [/sys/devices/LNXSYSTM:00/LNXSYBUS:00/TOS{1900,620{0,7,8}}:00/touchpad](abi-testing.html#abi-sys-devices-lnxsystm-00-lnxsybus-00-tos-1900-620-0-7-8-00-touchpad)
* [/sys/devices/LNXSYSTM:00/LNXSYBUS:00/TOS{1900,620{0,7,8}}:00/available\_kbd\_modes](abi-testing.html#abi-sys-devices-lnxsystm-00-lnxsybus-00-tos-1900-620-0-7-8-00-available-kbd-modes)
* [/sys/devices/LNXSYSTM:00/LNXSYBUS:00/TOS{1900,620{0,7,8}}:00/kbd\_type](abi-testing.html#abi-sys-devices-lnxsystm-00-lnxsybus-00-tos-1900-620-0-7-8-00-kbd-type)
* [/sys/devices/LNXSYSTM:00/LNXSYBUS:00/TOS{1900,620{0,7,8}}:00/usb\_sleep\_charge](abi-testing.html#abi-sys-devices-lnxsystm-00-lnxsybus-00-tos-1900-620-0-7-8-00-usb-sleep-charge)
* [/sys/devices/LNXSYSTM:00/LNXSYBUS:00/TOS{1900,620{0,7,8}}:00/sleep\_functions\_on\_battery](abi-testing.html#abi-sys-devices-lnxsystm-00-lnxsybus-00-tos-1900-620-0-7-8-00-sleep-functions-on-battery)
* [/sys/devices/LNXSYSTM:00/LNXSYBUS:00/TOS{1900,620{0,7,8}}:00/usb\_rapid\_charge](abi-testing.html#abi-sys-devices-lnxsystm-00-lnxsybus-00-tos-1900-620-0-7-8-00-usb-rapid-charge)
* [/sys/devices/LNXSYSTM:00/LNXSYBUS:00/TOS{1900,620{0,7,8}}:00/usb\_sleep\_music](abi-testing.html#abi-sys-devices-lnxsystm-00-lnxsybus-00-tos-1900-620-0-7-8-00-usb-sleep-music)
* [/sys/devices/LNXSYSTM:00/LNXSYBUS:00/TOS{1900,620{0,7,8}}:00/version](abi-testing.html#abi-sys-devices-lnxsystm-00-lnxsybus-00-tos-1900-620-0-7-8-00-version)
* [/sys/devices/LNXSYSTM:00/LNXSYBUS:00/TOS{1900,620{0,7,8}}:00/fan](abi-testing.html#abi-sys-devices-lnxsystm-00-lnxsybus-00-tos-1900-620-0-7-8-00-fan)
* [/sys/devices/LNXSYSTM:00/LNXSYBUS:00/TOS{1900,620{0,7,8}}:00/kbd\_function\_keys](abi-testing.html#abi-sys-devices-lnxsystm-00-lnxsybus-00-tos-1900-620-0-7-8-00-kbd-function-keys)
* [/sys/devices/LNXSYSTM:00/LNXSYBUS:00/TOS{1900,620{0,7,8}}:00/panel\_power\_on](abi-testing.html#abi-sys-devices-lnxsystm-00-lnxsybus-00-tos-1900-620-0-7-8-00-panel-power-on)
* [/sys/devices/LNXSYSTM:00/LNXSYBUS:00/TOS{1900,620{0,7,8}}:00/usb\_three](abi-testing.html#abi-sys-devices-lnxsystm-00-lnxsybus-00-tos-1900-620-0-7-8-00-usb-three)
* [/sys/devices/LNXSYSTM:00/LNXSYBUS:00/TOS{1900,620{0,7,8}}:00/cooling\_method](abi-testing.html#abi-sys-devices-lnxsystm-00-lnxsybus-00-tos-1900-620-0-7-8-00-cooling-method)

## ABI file testing/sysfs-driver-toshiba\_haps

Has the following ABI:

* [/sys/devices/LNXSYSTM:00/LNXSYBUS:00/TOS620A:00/protection\_level](abi-testing.html#abi-sys-devices-lnxsystm-00-lnxsybus-00-tos620a-00-protection-level)
* [/sys/devices/LNXSYSTM:00/LNXSYBUS:00/TOS620A:00/reset\_protection](abi-testing.html#abi-sys-devices-lnxsystm-00-lnxsybus-00-tos620a-00-reset-protection)

## ABI file testing/sysfs-driver-typec-displayport

Has the following ABI:

* [/sys/bus/typec/devices/.../displayport/configuration](abi-testing.html#abi-sys-bus-typec-devices-displayport-configuration)
* [/sys/bus/typec/devices/.../displayport/pin\_assignment](abi-testing.html#abi-sys-bus-typec-devices-displayport-pin-assignment)
* [/sys/bus/typec/devices/.../displayport/hpd](abi-testing.html#abi-sys-bus-typec-devices-displayport-hpd)
* [/sys/bus/typec/devices/.../displayport/irq\_hpd](abi-testing.html#abi-sys-bus-typec-devices-displayport-irq-hpd)

## ABI file testing/sysfs-driver-uacce

Has the following ABI:

* [/sys/class/uacce/<dev\_name>/api](abi-testing.html#abi-sys-class-uacce-dev-name-api)
* [/sys/class/uacce/<dev\_name>/flags](abi-testing.html#abi-sys-class-uacce-dev-name-flags)
* [/sys/class/uacce/<dev\_name>/available\_instances](abi-testing.html#abi-sys-class-uacce-dev-name-available-instances)
* [/sys/class/uacce/<dev\_name>/isolate\_strategy](abi-testing.html#abi-sys-class-uacce-dev-name-isolate-strategy)
* [/sys/class/uacce/<dev\_name>/isolate](abi-testing.html#abi-sys-class-uacce-dev-name-isolate)
* [/sys/class/uacce/<dev\_name>/algorithms](abi-testing.html#abi-sys-class-uacce-dev-name-algorithms)
* [/sys/class/uacce/<dev\_name>/region\_mmio\_size](abi-testing.html#abi-sys-class-uacce-dev-name-region-mmio-size)
* [/sys/class/uacce/<dev\_name>/region\_dus\_size](abi-testing.html#abi-sys-class-uacce-dev-name-region-dus-size)

## ABI file testing/sysfs-driver-ucsi-ccg

Has the following ABI:

* [/sys/bus/i2c/drivers/ucsi\_ccg/.../do\_flash](abi-testing.html#abi-sys-bus-i2c-drivers-ucsi-ccg-do-flash)

## ABI file testing/sysfs-driver-ufs

Has the following ABI:

* [/sys/bus/\*/drivers/ufshcd/\*/auto\_hibern8](abi-testing.html#abi-sys-bus-drivers-ufshcd-auto-hibern8)
* [/sys/bus/platform/drivers/ufshcd/\*/device\_descriptor/device\_type](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-device-type)
* [/sys/bus/platform/devices/\*.ufs/device\_descriptor/device\_type](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-device-type)
* [/sys/bus/platform/drivers/ufshcd/\*/device\_descriptor/device\_class](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-device-class)
* [/sys/bus/platform/devices/\*.ufs/device\_descriptor/device\_class](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-device-class)
* [/sys/bus/platform/drivers/ufshcd/\*/device\_descriptor/device\_sub\_class](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-device-sub-class)
* [/sys/bus/platform/devices/\*.ufs/device\_descriptor/device\_sub\_class](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-device-sub-class)
* [/sys/bus/platform/drivers/ufshcd/\*/device\_descriptor/protocol](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-protocol)
* [/sys/bus/platform/devices/\*.ufs/device\_descriptor/protocol](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-protocol)
* [/sys/bus/platform/drivers/ufshcd/\*/device\_descriptor/number\_of\_luns](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-number-of-luns)
* [/sys/bus/platform/devices/\*.ufs/device\_descriptor/number\_of\_luns](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-number-of-luns)
* [/sys/bus/platform/drivers/ufshcd/\*/device\_descriptor/number\_of\_wluns](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-number-of-wluns)
* [/sys/bus/platform/devices/\*.ufs/device\_descriptor/number\_of\_wluns](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-number-of-wluns)
* [/sys/bus/platform/drivers/ufshcd/\*/device\_descriptor/boot\_enable](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-boot-enable)
* [/sys/bus/platform/devices/\*.ufs/device\_descriptor/boot\_enable](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-boot-enable)
* [/sys/bus/platform/drivers/ufshcd/\*/device\_descriptor/descriptor\_access\_enable](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-descriptor-access-enable)
* [/sys/bus/platform/devices/\*.ufs/device\_descriptor/descriptor\_access\_enable](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-descriptor-access-enable)
* [/sys/bus/platform/drivers/ufshcd/\*/device\_descriptor/initial\_power\_mode](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-initial-power-mode)
* [/sys/bus/platform/devices/\*.ufs/device\_descriptor/initial\_power\_mode](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-initial-power-mode)
* [/sys/bus/platform/drivers/ufshcd/\*/device\_descriptor/high\_priority\_lun](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-high-priority-lun)
* [/sys/bus/platform/devices/\*.ufs/device\_descriptor/high\_priority\_lun](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-high-priority-lun)
* [/sys/bus/platform/drivers/ufshcd/\*/device\_descriptor/secure\_removal\_type](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-secure-removal-type)
* [/sys/bus/platform/devices/\*.ufs/device\_descriptor/secure\_removal\_type](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-secure-removal-type)
* [/sys/bus/platform/drivers/ufshcd/\*/device\_descriptor/support\_security\_lun](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-support-security-lun)
* [/sys/bus/platform/devices/\*.ufs/device\_descriptor/support\_security\_lun](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-support-security-lun)
* [/sys/bus/platform/drivers/ufshcd/\*/device\_descriptor/bkops\_termination\_latency](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-bkops-termination-latency)
* [/sys/bus/platform/devices/\*.ufs/device\_descriptor/bkops\_termination\_latency](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-bkops-termination-latency)
* [/sys/bus/platform/drivers/ufshcd/\*/device\_descriptor/initial\_active\_icc\_level](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-initial-active-icc-level)
* [/sys/bus/platform/devices/\*.ufs/device\_descriptor/initial\_active\_icc\_level](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-initial-active-icc-level)
* [/sys/bus/platform/drivers/ufshcd/\*/device\_descriptor/specification\_version](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-specification-version)
* [/sys/bus/platform/devices/\*.ufs/device\_descriptor/specification\_version](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-specification-version)
* [/sys/bus/platform/drivers/ufshcd/\*/device\_descriptor/manufacturing\_date](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-manufacturing-date)
* [/sys/bus/platform/devices/\*.ufs/device\_descriptor/manufacturing\_date](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-manufacturing-date)
* [/sys/bus/platform/drivers/ufshcd/\*/device\_descriptor/manufacturer\_id](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-manufacturer-id)
* [/sys/bus/platform/devices/\*.ufs/device\_descriptor/manufacturer\_id](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-manufacturer-id)
* [/sys/bus/platform/drivers/ufshcd/\*/device\_descriptor/rtt\_capability](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-rtt-capability)
* [/sys/bus/platform/devices/\*.ufs/device\_descriptor/rtt\_capability](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-rtt-capability)
* [/sys/bus/platform/drivers/ufshcd/\*/device\_descriptor/rtc\_update](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-rtc-update)
* [/sys/bus/platform/devices/\*.ufs/device\_descriptor/rtc\_update](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-rtc-update)
* [/sys/bus/platform/drivers/ufshcd/\*/device\_descriptor/ufs\_features](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-ufs-features)
* [/sys/bus/platform/devices/\*.ufs/device\_descriptor/ufs\_features](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-ufs-features)
* [/sys/bus/platform/drivers/ufshcd/\*/device\_descriptor/ffu\_timeout](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-ffu-timeout)
* [/sys/bus/platform/devices/\*.ufs/device\_descriptor/ffu\_timeout](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-ffu-timeout)
* [/sys/bus/platform/drivers/ufshcd/\*/device\_descriptor/queue\_depth](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-queue-depth)
* [/sys/bus/platform/devices/\*.ufs/device\_descriptor/queue\_depth](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-queue-depth)
* [/sys/bus/platform/drivers/ufshcd/\*/device\_descriptor/device\_version](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-device-version)
* [/sys/bus/platform/devices/\*.ufs/device\_descriptor/device\_version](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-device-version)
* [/sys/bus/platform/drivers/ufshcd/\*/device\_descriptor/number\_of\_secure\_wpa](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-number-of-secure-wpa)
* [/sys/bus/platform/devices/\*.ufs/device\_descriptor/number\_of\_secure\_wpa](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-number-of-secure-wpa)
* [/sys/bus/platform/drivers/ufshcd/\*/device\_descriptor/psa\_max\_data\_size](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-psa-max-data-size)
* [/sys/bus/platform/devices/\*.ufs/device\_descriptor/psa\_max\_data\_size](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-psa-max-data-size)
* [/sys/bus/platform/drivers/ufshcd/\*/device\_descriptor/psa\_state\_timeout](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-psa-state-timeout)
* [/sys/bus/platform/devices/\*.ufs/device\_descriptor/psa\_state\_timeout](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-psa-state-timeout)
* [/sys/bus/platform/drivers/ufshcd/\*/interconnect\_descriptor/unipro\_version](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-interconnect-descriptor-unipro-version)
* [/sys/bus/platform/devices/\*.ufs/interconnect\_descriptor/unipro\_version](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-interconnect-descriptor-unipro-version)
* [/sys/bus/platform/drivers/ufshcd/\*/interconnect\_descriptor/mphy\_version](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-interconnect-descriptor-mphy-version)
* [/sys/bus/platform/devices/\*.ufs/interconnect\_descriptor/mphy\_version](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-interconnect-descriptor-mphy-version)
* [/sys/bus/platform/drivers/ufshcd/\*/geometry\_descriptor/raw\_device\_capacity](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-raw-device-capacity)
* [/sys/bus/platform/devices/\*.ufs/geometry\_descriptor/raw\_device\_capacity](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-raw-device-capacity)
* [/sys/bus/platform/drivers/ufshcd/\*/geometry\_descriptor/max\_number\_of\_luns](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-max-number-of-luns)
* [/sys/bus/platform/devices/\*.ufs/geometry\_descriptor/max\_number\_of\_luns](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-max-number-of-luns)
* [/sys/bus/platform/drivers/ufshcd/\*/geometry\_descriptor/segment\_size](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-segment-size)
* [/sys/bus/platform/devices/\*.ufs/geometry\_descriptor/segment\_size](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-segment-size)
* [/sys/bus/platform/drivers/ufshcd/\*/geometry\_descriptor/allocation\_unit\_size](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-allocation-unit-size)
* [/sys/bus/platform/devices/\*.ufs/geometry\_descriptor/allocation\_unit\_size](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-allocation-unit-size)
* [/sys/bus/platform/drivers/ufshcd/\*/geometry\_descriptor/min\_addressable\_block\_size](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-min-addressable-block-size)
* [/sys/bus/platform/devices/\*.ufs/geometry\_descriptor/min\_addressable\_block\_size](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-min-addressable-block-size)
* [/sys/bus/platform/drivers/ufshcd/\*/geometry\_descriptor/optimal\_read\_block\_size](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-optimal-read-block-size)
* [/sys/bus/platform/devices/\*.ufs/geometry\_descriptor/optimal\_read\_block\_size](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-optimal-read-block-size)
* [/sys/bus/platform/drivers/ufshcd/\*/geometry\_descriptor/optimal\_write\_block\_size](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-optimal-write-block-size)
* [/sys/bus/platform/devices/\*.ufs/geometry\_descriptor/optimal\_write\_block\_size](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-optimal-write-block-size)
* [/sys/bus/platform/drivers/ufshcd/\*/geometry\_descriptor/max\_in\_buffer\_size](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-max-in-buffer-size)
* [/sys/bus/platform/devices/\*.ufs/geometry\_descriptor/max\_in\_buffer\_size](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-max-in-buffer-size)
* [/sys/bus/platform/drivers/ufshcd/\*/geometry\_descriptor/max\_out\_buffer\_size](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-max-out-buffer-size)
* [/sys/bus/platform/devices/\*.ufs/geometry\_descriptor/max\_out\_buffer\_size](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-max-out-buffer-size)
* [/sys/bus/platform/drivers/ufshcd/\*/geometry\_descriptor/rpmb\_rw\_size](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-rpmb-rw-size)
* [/sys/bus/platform/devices/\*.ufs/geometry\_descriptor/rpmb\_rw\_size](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-rpmb-rw-size)
* [/sys/bus/platform/drivers/ufshcd/\*/geometry\_descriptor/dyn\_capacity\_resource\_policy](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-dyn-capacity-resource-policy)
* [/sys/bus/platform/devices/\*.ufs/geometry\_descriptor/dyn\_capacity\_resource\_policy](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-dyn-capacity-resource-policy)
* [/sys/bus/platform/drivers/ufshcd/\*/geometry\_descriptor/data\_ordering](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-data-ordering)
* [/sys/bus/platform/devices/\*.ufs/geometry\_descriptor/data\_ordering](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-data-ordering)
* [/sys/bus/platform/drivers/ufshcd/\*/geometry\_descriptor/max\_number\_of\_contexts](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-max-number-of-contexts)
* [/sys/bus/platform/devices/\*.ufs/geometry\_descriptor/max\_number\_of\_contexts](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-max-number-of-contexts)
* [/sys/bus/platform/drivers/ufshcd/\*/geometry\_descriptor/sys\_data\_tag\_unit\_size](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-sys-data-tag-unit-size)
* [/sys/bus/platform/devices/\*.ufs/geometry\_descriptor/sys\_data\_tag\_unit\_size](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-sys-data-tag-unit-size)
* [/sys/bus/platform/drivers/ufshcd/\*/geometry\_descriptor/sys\_data\_tag\_resource\_size](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-sys-data-tag-resource-size)
* [/sys/bus/platform/devices/\*.ufs/geometry\_descriptor/sys\_data\_tag\_resource\_size](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-sys-data-tag-resource-size)
* [/sys/bus/platform/drivers/ufshcd/\*/geometry\_descriptor/secure\_removal\_types](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-secure-removal-types)
* [/sys/bus/platform/devices/\*.ufs/geometry\_descriptor/secure\_removal\_types](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-secure-removal-types)
* [/sys/bus/platform/drivers/ufshcd/\*/geometry\_descriptor/memory\_types](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-memory-types)
* [/sys/bus/platform/devices/\*.ufs/geometry\_descriptor/memory\_types](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-memory-types)
* [/sys/bus/platform/drivers/ufshcd/\*/geometry\_descriptor/\*\_memory\_max\_alloc\_units](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-memory-max-alloc-units)
* [/sys/bus/platform/devices/\*.ufs/geometry\_descriptor/\*\_memory\_max\_alloc\_units](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-memory-max-alloc-units)
* [/sys/bus/platform/drivers/ufshcd/\*/geometry\_descriptor/\*\_memory\_capacity\_adjustment\_factor](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-memory-capacity-adjustment-factor)
* [/sys/bus/platform/devices/\*.ufs/geometry\_descriptor/\*\_memory\_capacity\_adjustment\_factor](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-memory-capacity-adjustment-factor)
* [/sys/bus/platform/drivers/ufshcd/\*/health\_descriptor/eol\_info](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-health-descriptor-eol-info)
* [/sys/bus/platform/devices/\*.ufs/health\_descriptor/eol\_info](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-health-descriptor-eol-info)
* [/sys/bus/platform/drivers/ufshcd/\*/health\_descriptor/life\_time\_estimation\_a](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-health-descriptor-life-time-estimation-a)
* [/sys/bus/platform/devices/\*.ufs/health\_descriptor/life\_time\_estimation\_a](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-health-descriptor-life-time-estimation-a)
* [/sys/bus/platform/drivers/ufshcd/\*/health\_descriptor/life\_time\_estimation\_b](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-health-descriptor-life-time-estimation-b)
* [/sys/bus/platform/devices/\*.ufs/health\_descriptor/life\_time\_estimation\_b](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-health-descriptor-life-time-estimation-b)
* [/sys/bus/platform/drivers/ufshcd/\*/power\_descriptor/active\_icc\_levels\_vcc\*](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-power-descriptor-active-icc-levels-vcc)
* [/sys/bus/platform/devices/\*.ufs/power\_descriptor/active\_icc\_levels\_vcc\*](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-power-descriptor-active-icc-levels-vcc)
* [/sys/bus/platform/drivers/ufshcd/\*/string\_descriptors/manufacturer\_name](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-string-descriptors-manufacturer-name)
* [/sys/bus/platform/devices/\*.ufs/string\_descriptors/manufacturer\_name](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-string-descriptors-manufacturer-name)
* [/sys/bus/platform/drivers/ufshcd/\*/string\_descriptors/product\_name](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-string-descriptors-product-name)
* [/sys/bus/platform/devices/\*.ufs/string\_descriptors/product\_name](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-string-descriptors-product-name)
* [/sys/bus/platform/drivers/ufshcd/\*/string\_descriptors/oem\_id](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-string-descriptors-oem-id)
* [/sys/bus/platform/devices/\*.ufs/string\_descriptors/oem\_id](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-string-descriptors-oem-id)
* [/sys/bus/platform/drivers/ufshcd/\*/string\_descriptors/serial\_number](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-string-descriptors-serial-number)
* [/sys/bus/platform/devices/\*.ufs/string\_descriptors/serial\_number](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-string-descriptors-serial-number)
* [/sys/bus/platform/drivers/ufshcd/\*/string\_descriptors/product\_revision](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-string-descriptors-product-revision)
* [/sys/bus/platform/devices/\*.ufs/string\_descriptors/product\_revision](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-string-descriptors-product-revision)
* [/sys/class/scsi\_device/\*/device/unit\_descriptor/boot\_lun\_id](abi-testing.html#abi-sys-class-scsi-device-device-unit-descriptor-boot-lun-id)
* [/sys/class/scsi\_device/\*/device/unit\_descriptor/lun\_write\_protect](abi-testing.html#abi-sys-class-scsi-device-device-unit-descriptor-lun-write-protect)
* [/sys/class/scsi\_device/\*/device/unit\_descriptor/lun\_queue\_depth](abi-testing.html#abi-sys-class-scsi-device-device-unit-descriptor-lun-queue-depth)
* [/sys/class/scsi\_device/\*/device/unit\_descriptor/psa\_sensitive](abi-testing.html#abi-sys-class-scsi-device-device-unit-descriptor-psa-sensitive)
* [/sys/class/scsi\_device/\*/device/unit\_descriptor/lun\_memory\_type](abi-testing.html#abi-sys-class-scsi-device-device-unit-descriptor-lun-memory-type)
* [/sys/class/scsi\_device/\*/device/unit\_descriptor/data\_reliability](abi-testing.html#abi-sys-class-scsi-device-device-unit-descriptor-data-reliability)
* [/sys/class/scsi\_device/\*/device/unit\_descriptor/logical\_block\_size](abi-testing.html#abi-sys-class-scsi-device-device-unit-descriptor-logical-block-size)
* [/sys/class/scsi\_device/\*/device/unit\_descriptor/logical\_block\_count](abi-testing.html#abi-sys-class-scsi-device-device-unit-descriptor-logical-block-count)
* [/sys/class/scsi\_device/\*/device/unit\_descriptor/erase\_block\_size](abi-testing.html#abi-sys-class-scsi-device-device-unit-descriptor-erase-block-size)
* [/sys/class/scsi\_device/\*/device/unit\_descriptor/provisioning\_type](abi-testing.html#abi-sys-class-scsi-device-device-unit-descriptor-provisioning-type)
* [/sys/class/scsi\_device/\*/device/unit\_descriptor/physical\_memory\_resource\_count](abi-testing.html#abi-sys-class-scsi-device-device-unit-descriptor-physical-memory-resource-count)
* [/sys/class/scsi\_device/\*/device/unit\_descriptor/context\_capabilities](abi-testing.html#abi-sys-class-scsi-device-device-unit-descriptor-context-capabilities)
* [/sys/class/scsi\_device/\*/device/unit\_descriptor/large\_unit\_granularity](abi-testing.html#abi-sys-class-scsi-device-device-unit-descriptor-large-unit-granularity)
* [/sys/bus/platform/drivers/ufshcd/\*/flags/device\_init](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-flags-device-init)
* [/sys/bus/platform/devices/\*.ufs/flags/device\_init](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-flags-device-init)
* [/sys/bus/platform/drivers/ufshcd/\*/flags/permanent\_wpe](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-flags-permanent-wpe)
* [/sys/bus/platform/devices/\*.ufs/flags/permanent\_wpe](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-flags-permanent-wpe)
* [/sys/bus/platform/drivers/ufshcd/\*/flags/power\_on\_wpe](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-flags-power-on-wpe)
* [/sys/bus/platform/devices/\*.ufs/flags/power\_on\_wpe](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-flags-power-on-wpe)
* [/sys/bus/platform/drivers/ufshcd/\*/flags/bkops\_enable](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-flags-bkops-enable)
* [/sys/bus/platform/devices/\*.ufs/flags/bkops\_enable](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-flags-bkops-enable)
* [/sys/bus/platform/drivers/ufshcd/\*/flags/life\_span\_mode\_enable](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-flags-life-span-mode-enable)
* [/sys/bus/platform/devices/\*.ufs/flags/life\_span\_mode\_enable](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-flags-life-span-mode-enable)
* [/sys/bus/platform/drivers/ufshcd/\*/flags/phy\_resource\_removal](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-flags-phy-resource-removal)
* [/sys/bus/platform/devices/\*.ufs/flags/phy\_resource\_removal](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-flags-phy-resource-removal)
* [/sys/bus/platform/drivers/ufshcd/\*/flags/busy\_rtc](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-flags-busy-rtc)
* [/sys/bus/platform/devices/\*.ufs/flags/busy\_rtc](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-flags-busy-rtc)
* [/sys/bus/platform/drivers/ufshcd/\*/flags/disable\_fw\_update](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-flags-disable-fw-update)
* [/sys/bus/platform/devices/\*.ufs/flags/disable\_fw\_update](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-flags-disable-fw-update)
* [/sys/bus/platform/drivers/ufshcd/\*/attributes/boot\_lun\_enabled](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-attributes-boot-lun-enabled)
* [/sys/bus/platform/devices/\*.ufs/attributes/boot\_lun\_enabled](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-attributes-boot-lun-enabled)
* [/sys/bus/platform/drivers/ufshcd/\*/attributes/current\_power\_mode](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-attributes-current-power-mode)
* [/sys/bus/platform/devices/\*.ufs/attributes/current\_power\_mode](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-attributes-current-power-mode)
* [/sys/bus/platform/drivers/ufshcd/\*/attributes/active\_icc\_level](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-attributes-active-icc-level)
* [/sys/bus/platform/devices/\*.ufs/attributes/active\_icc\_level](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-attributes-active-icc-level)
* [/sys/bus/platform/drivers/ufshcd/\*/attributes/ooo\_data\_enabled](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-attributes-ooo-data-enabled)
* [/sys/bus/platform/devices/\*.ufs/attributes/ooo\_data\_enabled](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-attributes-ooo-data-enabled)
* [/sys/bus/platform/drivers/ufshcd/\*/attributes/bkops\_status](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-attributes-bkops-status)
* [/sys/bus/platform/devices/\*.ufs/attributes/bkops\_status](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-attributes-bkops-status)
* [/sys/bus/platform/drivers/ufshcd/\*/attributes/purge\_status](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-attributes-purge-status)
* [/sys/bus/platform/devices/\*.ufs/attributes/purge\_status](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-attributes-purge-status)
* [/sys/bus/platform/drivers/ufshcd/\*/attributes/max\_data\_in\_size](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-attributes-max-data-in-size)
* [/sys/bus/platform/devices/\*.ufs/attributes/max\_data\_in\_size](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-attributes-max-data-in-size)
* [/sys/bus/platform/drivers/ufshcd/\*/attributes/max\_data\_out\_size](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-attributes-max-data-out-size)
* [/sys/bus/platform/devices/\*.ufs/attributes/max\_data\_out\_size](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-attributes-max-data-out-size)
* [/sys/bus/platform/drivers/ufshcd/\*/attributes/reference\_clock\_frequency](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-attributes-reference-clock-frequency)
* [/sys/bus/platform/devices/\*.ufs/attributes/reference\_clock\_frequency](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-attributes-reference-clock-frequency)
* [/sys/bus/platform/drivers/ufshcd/\*/attributes/configuration\_descriptor\_lock](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-attributes-configuration-descriptor-lock)
* [/sys/bus/platform/devices/\*.ufs/attributes/configuration\_descriptor\_lock](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-attributes-configuration-descriptor-lock)
* [/sys/bus/platform/drivers/ufshcd/\*/attributes/max\_number\_of\_rtt](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-attributes-max-number-of-rtt)
* [/sys/bus/platform/devices/\*.ufs/attributes/max\_number\_of\_rtt](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-attributes-max-number-of-rtt)
* [/sys/bus/platform/drivers/ufshcd/\*/attributes/exception\_event\_control](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-attributes-exception-event-control)
* [/sys/bus/platform/devices/\*.ufs/attributes/exception\_event\_control](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-attributes-exception-event-control)
* [/sys/bus/platform/drivers/ufshcd/\*/attributes/exception\_event\_status](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-attributes-exception-event-status)
* [/sys/bus/platform/devices/\*.ufs/attributes/exception\_event\_status](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-attributes-exception-event-status)
* [/sys/bus/platform/drivers/ufshcd/\*/attributes/ffu\_status](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-attributes-ffu-status)
* [/sys/bus/platform/devices/\*.ufs/attributes/ffu\_status](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-attributes-ffu-status)
* [/sys/bus/platform/drivers/ufshcd/\*/attributes/psa\_state](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-attributes-psa-state)
* [/sys/bus/platform/devices/\*.ufs/attributes/psa\_state](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-attributes-psa-state)
* [/sys/bus/platform/drivers/ufshcd/\*/attributes/psa\_data\_size](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-attributes-psa-data-size)
* [/sys/bus/platform/devices/\*.ufs/attributes/psa\_data\_size](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-attributes-psa-data-size)
* [/sys/class/scsi\_device/\*/device/dyn\_cap\_needed](abi-testing.html#abi-sys-class-scsi-device-device-dyn-cap-needed)
* [/sys/bus/platform/drivers/ufshcd/\*/rpm\_lvl](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-rpm-lvl)
* [/sys/bus/platform/devices/\*.ufs/rpm\_lvl](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-rpm-lvl)
* [/sys/bus/platform/drivers/ufshcd/\*/rpm\_target\_dev\_state](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-rpm-target-dev-state)
* [/sys/bus/platform/devices/\*.ufs/rpm\_target\_dev\_state](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-rpm-target-dev-state)
* [/sys/bus/platform/drivers/ufshcd/\*/rpm\_target\_link\_state](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-rpm-target-link-state)
* [/sys/bus/platform/devices/\*.ufs/rpm\_target\_link\_state](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-rpm-target-link-state)
* [/sys/bus/platform/drivers/ufshcd/\*/spm\_lvl](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-spm-lvl)
* [/sys/bus/platform/devices/\*.ufs/spm\_lvl](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-spm-lvl)
* [/sys/bus/platform/drivers/ufshcd/\*/spm\_target\_dev\_state](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-spm-target-dev-state)
* [/sys/bus/platform/devices/\*.ufs/spm\_target\_dev\_state](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-spm-target-dev-state)
* [/sys/bus/platform/drivers/ufshcd/\*/spm\_target\_link\_state](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-spm-target-link-state)
* [/sys/bus/platform/devices/\*.ufs/spm\_target\_link\_state](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-spm-target-link-state)
* [/sys/bus/platform/drivers/ufshcd/\*/monitor/monitor\_enable](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-monitor-monitor-enable)
* [/sys/bus/platform/devices/\*.ufs/monitor/monitor\_enable](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-monitor-monitor-enable)
* [/sys/bus/platform/drivers/ufshcd/\*/monitor/monitor\_chunk\_size](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-monitor-monitor-chunk-size)
* [/sys/bus/platform/devices/\*.ufs/monitor/monitor\_chunk\_size](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-monitor-monitor-chunk-size)
* [/sys/bus/platform/drivers/ufshcd/\*/monitor/read\_total\_sectors](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-monitor-read-total-sectors)
* [/sys/bus/platform/devices/\*.ufs/monitor/read\_total\_sectors](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-monitor-read-total-sectors)
* [/sys/bus/platform/drivers/ufshcd/\*/monitor/read\_total\_busy](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-monitor-read-total-busy)
* [/sys/bus/platform/devices/\*.ufs/monitor/read\_total\_busy](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-monitor-read-total-busy)
* [/sys/bus/platform/drivers/ufshcd/\*/monitor/read\_nr\_requests](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-monitor-read-nr-requests)
* [/sys/bus/platform/devices/\*.ufs/monitor/read\_nr\_requests](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-monitor-read-nr-requests)
* [/sys/bus/platform/drivers/ufshcd/\*/monitor/read\_req\_latency\_max](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-monitor-read-req-latency-max)
* [/sys/bus/platform/devices/\*.ufs/monitor/read\_req\_latency\_max](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-monitor-read-req-latency-max)
* [/sys/bus/platform/drivers/ufshcd/\*/monitor/read\_req\_latency\_min](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-monitor-read-req-latency-min)
* [/sys/bus/platform/devices/\*.ufs/monitor/read\_req\_latency\_min](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-monitor-read-req-latency-min)
* [/sys/bus/platform/drivers/ufshcd/\*/monitor/read\_req\_latency\_avg](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-monitor-read-req-latency-avg)
* [/sys/bus/platform/devices/\*.ufs/monitor/read\_req\_latency\_avg](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-monitor-read-req-latency-avg)
* [/sys/bus/platform/drivers/ufshcd/\*/monitor/read\_req\_latency\_sum](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-monitor-read-req-latency-sum)
* [/sys/bus/platform/devices/\*.ufs/monitor/read\_req\_latency\_sum](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-monitor-read-req-latency-sum)
* [/sys/bus/platform/drivers/ufshcd/\*/monitor/write\_total\_sectors](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-monitor-write-total-sectors)
* [/sys/bus/platform/devices/\*.ufs/monitor/write\_total\_sectors](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-monitor-write-total-sectors)
* [/sys/bus/platform/drivers/ufshcd/\*/monitor/write\_total\_busy](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-monitor-write-total-busy)
* [/sys/bus/platform/devices/\*.ufs/monitor/write\_total\_busy](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-monitor-write-total-busy)
* [/sys/bus/platform/drivers/ufshcd/\*/monitor/write\_nr\_requests](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-monitor-write-nr-requests)
* [/sys/bus/platform/devices/\*.ufs/monitor/write\_nr\_requests](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-monitor-write-nr-requests)
* [/sys/bus/platform/drivers/ufshcd/\*/monitor/write\_req\_latency\_max](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-monitor-write-req-latency-max)
* [/sys/bus/platform/devices/\*.ufs/monitor/write\_req\_latency\_max](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-monitor-write-req-latency-max)
* [/sys/bus/platform/drivers/ufshcd/\*/monitor/write\_req\_latency\_min](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-monitor-write-req-latency-min)
* [/sys/bus/platform/devices/\*.ufs/monitor/write\_req\_latency\_min](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-monitor-write-req-latency-min)
* [/sys/bus/platform/drivers/ufshcd/\*/monitor/write\_req\_latency\_avg](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-monitor-write-req-latency-avg)
* [/sys/bus/platform/devices/\*.ufs/monitor/write\_req\_latency\_avg](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-monitor-write-req-latency-avg)
* [/sys/bus/platform/drivers/ufshcd/\*/monitor/write\_req\_latency\_sum](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-monitor-write-req-latency-sum)
* [/sys/bus/platform/devices/\*.ufs/monitor/write\_req\_latency\_sum](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-monitor-write-req-latency-sum)
* [/sys/bus/platform/drivers/ufshcd/\*/power\_info/lane](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-power-info-lane)
* [/sys/bus/platform/devices/\*.ufs/power\_info/lane](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-power-info-lane)
* [/sys/bus/platform/drivers/ufshcd/\*/power\_info/mode](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-power-info-mode)
* [/sys/bus/platform/devices/\*.ufs/power\_info/mode](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-power-info-mode)
* [/sys/bus/platform/drivers/ufshcd/\*/power\_info/rate](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-power-info-rate)
* [/sys/bus/platform/devices/\*.ufs/power\_info/rate](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-power-info-rate)
* [/sys/bus/platform/drivers/ufshcd/\*/power\_info/gear](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-power-info-gear)
* [/sys/bus/platform/devices/\*.ufs/power\_info/gear](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-power-info-gear)
* [/sys/bus/platform/drivers/ufshcd/\*/power\_info/dev\_pm](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-power-info-dev-pm)
* [/sys/bus/platform/devices/\*.ufs/power\_info/dev\_pm](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-power-info-dev-pm)
* [/sys/bus/platform/drivers/ufshcd/\*/power\_info/link\_state](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-power-info-link-state)
* [/sys/bus/platform/devices/\*.ufs/power\_info/link\_state](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-power-info-link-state)
* [/sys/bus/platform/drivers/ufshcd/\*/device\_descriptor/wb\_presv\_us\_en](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-wb-presv-us-en)
* [/sys/bus/platform/devices/\*.ufs/device\_descriptor/wb\_presv\_us\_en](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-wb-presv-us-en)
* [/sys/bus/platform/drivers/ufshcd/\*/device\_descriptor/wb\_shared\_alloc\_units](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-wb-shared-alloc-units)
* [/sys/bus/platform/devices/\*.ufs/device\_descriptor/wb\_shared\_alloc\_units](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-wb-shared-alloc-units)
* [/sys/bus/platform/drivers/ufshcd/\*/device\_descriptor/wb\_type](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-wb-type)
* [/sys/bus/platform/devices/\*.ufs/device\_descriptor/wb\_type](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-descriptor-wb-type)
* [/sys/bus/platform/drivers/ufshcd/\*/geometry\_descriptor/wb\_buff\_cap\_adj](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-wb-buff-cap-adj)
* [/sys/bus/platform/devices/\*.ufs/geometry\_descriptor/wb\_buff\_cap\_adj](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-wb-buff-cap-adj)
* [/sys/bus/platform/drivers/ufshcd/\*/geometry\_descriptor/wb\_max\_alloc\_units](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-wb-max-alloc-units)
* [/sys/bus/platform/devices/\*.ufs/geometry\_descriptor/wb\_max\_alloc\_units](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-wb-max-alloc-units)
* [/sys/bus/platform/drivers/ufshcd/\*/geometry\_descriptor/wb\_max\_wb\_luns](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-wb-max-wb-luns)
* [/sys/bus/platform/devices/\*.ufs/geometry\_descriptor/wb\_max\_wb\_luns](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-wb-max-wb-luns)
* [/sys/bus/platform/drivers/ufshcd/\*/geometry\_descriptor/wb\_sup\_red\_type](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-wb-sup-red-type)
* [/sys/bus/platform/devices/\*.ufs/geometry\_descriptor/wb\_sup\_red\_type](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-wb-sup-red-type)
* [/sys/bus/platform/drivers/ufshcd/\*/geometry\_descriptor/wb\_sup\_wb\_type](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-wb-sup-wb-type)
* [/sys/bus/platform/devices/\*.ufs/geometry\_descriptor/wb\_sup\_wb\_type](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-geometry-descriptor-wb-sup-wb-type)
* [/sys/bus/platform/drivers/ufshcd/\*/flags/wb\_enable](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-flags-wb-enable)
* [/sys/bus/platform/devices/\*.ufs/flags/wb\_enable](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-flags-wb-enable)
* [/sys/bus/platform/drivers/ufshcd/\*/flags/wb\_flush\_en](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-flags-wb-flush-en)
* [/sys/bus/platform/devices/\*.ufs/flags/wb\_flush\_en](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-flags-wb-flush-en)
* [/sys/bus/platform/drivers/ufshcd/\*/flags/wb\_flush\_during\_h8](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-flags-wb-flush-during-h8)
* [/sys/bus/platform/devices/\*.ufs/flags/wb\_flush\_during\_h8](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-flags-wb-flush-during-h8)
* [/sys/bus/platform/drivers/ufshcd/\*/attributes/wb\_avail\_buf](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-attributes-wb-avail-buf)
* [/sys/bus/platform/devices/\*.ufs/attributes/wb\_avail\_buf](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-attributes-wb-avail-buf)
* [/sys/bus/platform/drivers/ufshcd/\*/attributes/wb\_cur\_buf](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-attributes-wb-cur-buf)
* [/sys/bus/platform/devices/\*.ufs/attributes/wb\_cur\_buf](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-attributes-wb-cur-buf)
* [/sys/bus/platform/drivers/ufshcd/\*/attributes/wb\_flush\_status](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-attributes-wb-flush-status)
* [/sys/bus/platform/devices/\*.ufs/attributes/wb\_flush\_status](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-attributes-wb-flush-status)
* [/sys/bus/platform/drivers/ufshcd/\*/attributes/wb\_life\_time\_est](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-attributes-wb-life-time-est)
* [/sys/bus/platform/devices/\*.ufs/attributes/wb\_life\_time\_est](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-attributes-wb-life-time-est)
* [/sys/class/scsi\_device/\*/device/unit\_descriptor/wb\_buf\_alloc\_units](abi-testing.html#abi-sys-class-scsi-device-device-unit-descriptor-wb-buf-alloc-units)
* [/sys/bus/platform/drivers/ufshcd/\*/wb\_on](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-wb-on)
* [/sys/bus/platform/devices/\*.ufs/wb\_on](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-wb-on)
* [/sys/bus/platform/drivers/ufshcd/\*/enable\_wb\_buf\_flush](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-enable-wb-buf-flush)
* [/sys/bus/platform/devices/\*.ufs/enable\_wb\_buf\_flush](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-enable-wb-buf-flush)
* [/sys/bus/platform/drivers/ufshcd/\*/wb\_flush\_threshold](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-wb-flush-threshold)
* [/sys/bus/platform/devices/\*.ufs/wb\_flush\_threshold](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-wb-flush-threshold)
* [/sys/bus/platform/drivers/ufshcd/\*/capabilities/](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-capabilities)
* [/sys/bus/platform/devices/\*.ufs/capabilities/](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-capabilities)
* [/sys/bus/platform/drivers/ufshcd/\*/capabilities/clock\_scaling](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-capabilities-clock-scaling)
* [/sys/bus/platform/devices/\*.ufs/capabilities/clock\_scaling](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-capabilities-clock-scaling)
* [/sys/bus/platform/drivers/ufshcd/\*/capabilities/write\_booster](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-capabilities-write-booster)
* [/sys/bus/platform/devices/\*.ufs/capabilities/write\_booster](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-capabilities-write-booster)
* [/sys/bus/platform/drivers/ufshcd/\*/rtc\_update\_ms](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-rtc-update-ms)
* [/sys/bus/platform/devices/\*.ufs/rtc\_update\_ms](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-rtc-update-ms)
* [/sys/devices/platform/.../ufshci\_capabilities/version](abi-testing.html#abi-sys-devices-platform-ufshci-capabilities-version)
* [/sys/devices/platform/.../ufshci\_capabilities/product\_id](abi-testing.html#abi-sys-devices-platform-ufshci-capabilities-product-id)
* [/sys/devices/platform/.../ufshci\_capabilities/man\_id](abi-testing.html#abi-sys-devices-platform-ufshci-capabilities-man-id)
* [/sys/bus/platform/drivers/ufshcd/\*/critical\_health](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-critical-health)
* [/sys/bus/platform/devices/\*.ufs/critical\_health](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-critical-health)
* [/sys/bus/platform/drivers/ufshcd/\*/clkscale\_enable](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-clkscale-enable)
* [/sys/bus/platform/devices/\*.ufs/clkscale\_enable](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-clkscale-enable)
* [/sys/bus/platform/drivers/ufshcd/\*/clkgate\_enable](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-clkgate-enable)
* [/sys/bus/platform/devices/\*.ufs/clkgate\_enable](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-clkgate-enable)
* [/sys/bus/platform/drivers/ufshcd/\*/clkgate\_delay\_ms](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-clkgate-delay-ms)
* [/sys/bus/platform/devices/\*.ufs/clkgate\_delay\_ms](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-clkgate-delay-ms)
* [/sys/bus/platform/drivers/ufshcd/\*/device\_lvl\_exception\_count](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-lvl-exception-count)
* [/sys/bus/platform/devices/\*.ufs/device\_lvl\_exception\_count](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-lvl-exception-count)
* [/sys/bus/platform/drivers/ufshcd/\*/device\_lvl\_exception\_id](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-lvl-exception-id)
* [/sys/bus/platform/devices/\*.ufs/device\_lvl\_exception\_id](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-device-lvl-exception-id)
* [/sys/bus/platform/drivers/ufshcd/\*/wb\_resize\_enable](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-wb-resize-enable)
* [/sys/bus/platform/devices/\*.ufs/wb\_resize\_enable](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-wb-resize-enable)
* [/sys/bus/platform/drivers/ufshcd/\*/attributes/wb\_resize\_hint](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-attributes-wb-resize-hint)
* [/sys/bus/platform/devices/\*.ufs/attributes/wb\_resize\_hint](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-attributes-wb-resize-hint)
* [/sys/bus/platform/drivers/ufshcd/\*/attributes/wb\_resize\_status](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-attributes-wb-resize-status)
* [/sys/bus/platform/devices/\*.ufs/attributes/wb\_resize\_status](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-attributes-wb-resize-status)
* [/sys/bus/platform/drivers/ufshcd/\*/hid/analysis\_trigger](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-hid-analysis-trigger)
* [/sys/bus/platform/devices/\*.ufs/hid/analysis\_trigger](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-hid-analysis-trigger)
* [/sys/bus/platform/drivers/ufshcd/\*/hid/defrag\_trigger](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-hid-defrag-trigger)
* [/sys/bus/platform/devices/\*.ufs/hid/defrag\_trigger](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-hid-defrag-trigger)
* [/sys/bus/platform/drivers/ufshcd/\*/hid/fragmented\_size](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-hid-fragmented-size)
* [/sys/bus/platform/devices/\*.ufs/hid/fragmented\_size](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-hid-fragmented-size)
* [/sys/bus/platform/drivers/ufshcd/\*/hid/defrag\_size](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-hid-defrag-size)
* [/sys/bus/platform/devices/\*.ufs/hid/defrag\_size](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-hid-defrag-size)
* [/sys/bus/platform/drivers/ufshcd/\*/hid/progress\_ratio](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-hid-progress-ratio)
* [/sys/bus/platform/devices/\*.ufs/hid/progress\_ratio](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-hid-progress-ratio)
* [/sys/bus/platform/drivers/ufshcd/\*/hid/state](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-hid-state)
* [/sys/bus/platform/devices/\*.ufs/hid/state](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-hid-state)
* [/sys/bus/platform/drivers/ufshcd/\*/dme\_qos\_notification](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-dme-qos-notification)
* [/sys/bus/platform/devices/\*.ufs/dme\_qos\_notification](abi-testing.html#abi-sys-bus-platform-drivers-ufshcd-dme-qos-notification)

## ABI file testing/sysfs-driver-uio\_pci\_sva-pasid

Has the following ABI:

* [/sys/bus/pci/drivers/uio\_pci\_sva/<pci\_dev>/pasid](abi-testing.html#abi-sys-bus-pci-drivers-uio-pci-sva-pci-dev-pasid)

## ABI file testing/sysfs-driver-uniwill-laptop

Has the following ABI:

* [/sys/bus/platform/devices/INOU0000:XX/fn\_lock](abi-testing.html#abi-sys-bus-platform-devices-inou0000-xx-fn-lock)
* [/sys/bus/platform/devices/INOU0000:XX/super\_key\_enable](abi-testing.html#abi-sys-bus-platform-devices-inou0000-xx-super-key-enable)
* [/sys/bus/platform/devices/INOU0000:XX/touchpad\_toggle\_enable](abi-testing.html#abi-sys-bus-platform-devices-inou0000-xx-touchpad-toggle-enable)
* [/sys/bus/platform/devices/INOU0000:XX/rainbow\_animation](abi-testing.html#abi-sys-bus-platform-devices-inou0000-xx-rainbow-animation)
* [/sys/bus/platform/devices/INOU0000:XX/breathing\_in\_suspend](abi-testing.html#abi-sys-bus-platform-devices-inou0000-xx-breathing-in-suspend)
* [/sys/bus/platform/devices/INOU0000:XX/ctgp\_offset](abi-testing.html#abi-sys-bus-platform-devices-inou0000-xx-ctgp-offset)
* [/sys/bus/platform/devices/INOU0000:XX/usb\_c\_power\_priority](abi-testing.html#abi-sys-bus-platform-devices-inou0000-xx-usb-c-power-priority)

## ABI file testing/sysfs-driver-w1\_ds28e17

Has the following ABI:

* [/sys/bus/w1/devices/19-<id>/speed](abi-testing.html#abi-sys-bus-w1-devices-19-id-speed)
* [/sys/bus/w1/devices/19-<id>/stretch](abi-testing.html#abi-sys-bus-w1-devices-19-id-stretch)

## ABI file testing/sysfs-driver-w1\_therm

Has the following ABI:

* [/sys/bus/w1/devices/.../alarms](abi-testing.html#abi-sys-bus-w1-devices-alarms)
* [/sys/bus/w1/devices/.../eeprom\_cmd](abi-testing.html#abi-sys-bus-w1-devices-eeprom-cmd)
* [/sys/bus/w1/devices/.../ext\_power](abi-testing.html#abi-sys-bus-w1-devices-ext-power)
* [/sys/bus/w1/devices/.../resolution](abi-testing.html#abi-sys-bus-w1-devices-resolution)
* [/sys/bus/w1/devices/.../temperature](abi-testing.html#abi-sys-bus-w1-devices-temperature)
* [/sys/bus/w1/devices/.../w1\_slave](abi-testing.html#abi-sys-bus-w1-devices-w1-slave)
* [/sys/bus/w1/devices/w1\_bus\_masterXX/therm\_bulk\_read](abi-testing.html#abi-sys-bus-w1-devices-w1-bus-masterxx-therm-bulk-read)
* [/sys/bus/w1/devices/.../conv\_time](abi-testing.html#abi-sys-bus-w1-devices-conv-time)
* [/sys/bus/w1/devices/.../features](abi-testing.html#abi-sys-bus-w1-devices-features)

## ABI file testing/sysfs-driver-wacom

Has the following ABI:

* [/sys/bus/hid/devices/<bus>:<vid>:<pid>.<n>/speed](abi-testing.html#abi-sys-bus-hid-devices-bus-vid-pid-n-speed)
* [/sys/bus/hid/devices/<bus>:<vid>:<pid>.<n>/wacom\_led/led](abi-testing.html#abi-sys-bus-hid-devices-bus-vid-pid-n-wacom-led-led)
* [/sys/bus/hid/devices/<bus>:<vid>:<pid>.<n>/wacom\_led/status0\_luminance](abi-testing.html#abi-sys-bus-hid-devices-bus-vid-pid-n-wacom-led-status0-luminance)
* [/sys/bus/hid/devices/<bus>:<vid>:<pid>.<n>/wacom\_led/status1\_luminance](abi-testing.html#abi-sys-bus-hid-devices-bus-vid-pid-n-wacom-led-status1-luminance)
* [/sys/bus/hid/devices/<bus>:<vid>:<pid>.<n>/wacom\_led/status\_led0\_select](abi-testing.html#abi-sys-bus-hid-devices-bus-vid-pid-n-wacom-led-status-led0-select)
* [/sys/bus/hid/devices/<bus>:<vid>:<pid>.<n>/wacom\_led/status\_led1\_select](abi-testing.html#abi-sys-bus-hid-devices-bus-vid-pid-n-wacom-led-status-led1-select)
* [/sys/bus/hid/devices/<bus>:<vid>:<pid>.<n>/wacom\_led/buttons\_luminance](abi-testing.html#abi-sys-bus-hid-devices-bus-vid-pid-n-wacom-led-buttons-luminance)
* [/sys/bus/hid/devices/<bus>:<vid>:<pid>.<n>/wacom\_led/button<n>\_rawimg](abi-testing.html#abi-sys-bus-hid-devices-bus-vid-pid-n-wacom-led-button-n-rawimg)
* [/sys/bus/hid/devices/<bus>:<vid>:<pid>.<n>/wacom\_remote/unpair\_remote](abi-testing.html#abi-sys-bus-hid-devices-bus-vid-pid-n-wacom-remote-unpair-remote)
* [/sys/bus/hid/devices/<bus>:<vid>:<pid>.<n>/wacom\_remote/<serial\_number>/remote\_mode](abi-testing.html#abi-sys-bus-hid-devices-bus-vid-pid-n-wacom-remote-serial-number-remote-mode)

## ABI file testing/sysfs-driver-xdata

Has the following ABI:

* [/sys/class/misc/drivers/dw-xdata-pcie.<device>/write](abi-testing.html#abi-sys-class-misc-drivers-dw-xdata-pcie-device-write)
* [/sys/class/misc/dw-xdata-pcie.<device>/read](abi-testing.html#abi-sys-class-misc-dw-xdata-pcie-device-read)

## ABI file testing/sysfs-driver-xen-blkback

Has the following ABI:

* [/sys/module/xen\_blkback/parameters/max\_buffer\_pages](abi-testing.html#abi-sys-module-xen-blkback-parameters-max-buffer-pages)
* [/sys/module/xen\_blkback/parameters/max\_persistent\_grants](abi-testing.html#abi-sys-module-xen-blkback-parameters-max-persistent-grants)
* [/sys/module/xen\_blkback/parameters/persistent\_grant\_unused\_seconds](abi-testing.html#abi-sys-module-xen-blkback-parameters-persistent-grant-unused-seconds)
* [/sys/module/xen\_blkback/parameters/buffer\_squeeze\_duration\_ms](abi-testing.html#abi-sys-module-xen-blkback-parameters-buffer-squeeze-duration-ms)
* [/sys/module/xen\_blkback/parameters/feature\_persistent](abi-testing.html#abi-sys-module-xen-blkback-parameters-feature-persistent)

## ABI file testing/sysfs-driver-xen-blkfront

Has the following ABI:

* [/sys/module/xen\_blkfront/parameters/max\_indirect\_segments](abi-testing.html#abi-sys-module-xen-blkfront-parameters-max-indirect-segments)
* [/sys/module/xen\_blkfront/parameters/feature\_persistent](abi-testing.html#abi-sys-module-xen-blkfront-parameters-feature-persistent)

## ABI file testing/sysfs-driver-xilinx-tmr-manager

Has the following ABI:

* [/sys/devices/platform/amba\_pl/<dev>/errcnt](abi-testing.html#abi-sys-devices-platform-amba-pl-dev-errcnt)
* [/sys/devices/platform/amba\_pl/<dev>/dis\_block\_break](abi-testing.html#abi-sys-devices-platform-amba-pl-dev-dis-block-break)

## ABI file testing/sysfs-driver-zynqmp-fpga

Has the following ABI:

* [/sys/bus/platform/drivers/zynqmp\_fpga\_manager/firmware:zynqmp-firmware:pcap/status](abi-testing.html#abi-sys-bus-platform-drivers-zynqmp-fpga-manager-firmware-zynqmp-firmware-pcap-status)

## ABI file testing/sysfs-edac-ecs

Has the following ABI:

* [/sys/bus/edac/devices/<dev-name>/ecs\_fruX](abi-testing.html#abi-sys-bus-edac-devices-dev-name-ecs-frux)
* [/sys/bus/edac/devices/<dev-name>/ecs\_fruX/log\_entry\_type](abi-testing.html#abi-sys-bus-edac-devices-dev-name-ecs-frux-log-entry-type)
* [/sys/bus/edac/devices/<dev-name>/ecs\_fruX/mode](abi-testing.html#abi-sys-bus-edac-devices-dev-name-ecs-frux-mode)
* [/sys/bus/edac/devices/<dev-name>/ecs\_fruX/reset](abi-testing.html#abi-sys-bus-edac-devices-dev-name-ecs-frux-reset)
* [/sys/bus/edac/devices/<dev-name>/ecs\_fruX/threshold](abi-testing.html#abi-sys-bus-edac-devices-dev-name-ecs-frux-threshold)

## ABI file testing/sysfs-edac-memory-repair

Has the following ABI:

* [/sys/bus/edac/devices/<dev-name>/mem\_repairX](abi-testing.html#abi-sys-bus-edac-devices-dev-name-mem-repairx)
* [/sys/bus/edac/devices/<dev-name>/mem\_repairX/repair\_type](abi-testing.html#abi-sys-bus-edac-devices-dev-name-mem-repairx-repair-type)
* [/sys/bus/edac/devices/<dev-name>/mem\_repairX/persist\_mode](abi-testing.html#abi-sys-bus-edac-devices-dev-name-mem-repairx-persist-mode)
* [/sys/bus/edac/devices/<dev-name>/mem\_repairX/repair\_safe\_when\_in\_use](abi-testing.html#abi-sys-bus-edac-devices-dev-name-mem-repairx-repair-safe-when-in-use)
* [/sys/bus/edac/devices/<dev-name>/mem\_repairX/hpa](abi-testing.html#abi-sys-bus-edac-devices-dev-name-mem-repairx-hpa)
* [/sys/bus/edac/devices/<dev-name>/mem\_repairX/dpa](abi-testing.html#abi-sys-bus-edac-devices-dev-name-mem-repairx-dpa)
* [/sys/bus/edac/devices/<dev-name>/mem\_repairX/nibble\_mask](abi-testing.html#abi-sys-bus-edac-devices-dev-name-mem-repairx-nibble-mask)
* [/sys/bus/edac/devices/<dev-name>/mem\_repairX/min\_hpa](abi-testing.html#abi-sys-bus-edac-devices-dev-name-mem-repairx-min-hpa)
* [/sys/bus/edac/devices/<dev-name>/mem\_repairX/max\_hpa](abi-testing.html#abi-sys-bus-edac-devices-dev-name-mem-repairx-min-hpa)
* [/sys/bus/edac/devices/<dev-name>/mem\_repairX/min\_dpa](abi-testing.html#abi-sys-bus-edac-devices-dev-name-mem-repairx-min-hpa)
* [/sys/bus/edac/devices/<dev-name>/mem\_repairX/max\_dpa](abi-testing.html#abi-sys-bus-edac-devices-dev-name-mem-repairx-min-hpa)
* [/sys/bus/edac/devices/<dev-name>/mem\_repairX/bank\_group](abi-testing.html#abi-sys-bus-edac-devices-dev-name-mem-repairx-bank-group)
* [/sys/bus/edac/devices/<dev-name>/mem\_repairX/bank](abi-testing.html#abi-sys-bus-edac-devices-dev-name-mem-repairx-bank-group)
* [/sys/bus/edac/devices/<dev-name>/mem\_repairX/rank](abi-testing.html#abi-sys-bus-edac-devices-dev-name-mem-repairx-bank-group)
* [/sys/bus/edac/devices/<dev-name>/mem\_repairX/row](abi-testing.html#abi-sys-bus-edac-devices-dev-name-mem-repairx-bank-group)
* [/sys/bus/edac/devices/<dev-name>/mem\_repairX/column](abi-testing.html#abi-sys-bus-edac-devices-dev-name-mem-repairx-bank-group)
* [/sys/bus/edac/devices/<dev-name>/mem\_repairX/channel](abi-testing.html#abi-sys-bus-edac-devices-dev-name-mem-repairx-bank-group)
* [/sys/bus/edac/devices/<dev-name>/mem\_repairX/sub\_channel](abi-testing.html#abi-sys-bus-edac-devices-dev-name-mem-repairx-bank-group)
* [/sys/bus/edac/devices/<dev-name>/mem\_repairX/repair](abi-testing.html#abi-sys-bus-edac-devices-dev-name-mem-repairx-repair)

## ABI file testing/sysfs-edac-scrub

Has the following ABI:

* [/sys/bus/edac/devices/<dev-name>/scrubX](abi-testing.html#abi-sys-bus-edac-devices-dev-name-scrubx)
* [/sys/bus/edac/devices/<dev-name>/scrubX/addr](abi-testing.html#abi-sys-bus-edac-devices-dev-name-scrubx-addr)
* [/sys/bus/edac/devices/<dev-name>/scrubX/size](abi-testing.html#abi-sys-bus-edac-devices-dev-name-scrubx-size)
* [/sys/bus/edac/devices/<dev-name>/scrubX/enable\_background](abi-testing.html#abi-sys-bus-edac-devices-dev-name-scrubx-enable-background)
* [/sys/bus/edac/devices/<dev-name>/scrubX/min\_cycle\_duration](abi-testing.html#abi-sys-bus-edac-devices-dev-name-scrubx-min-cycle-duration)
* [/sys/bus/edac/devices/<dev-name>/scrubX/max\_cycle\_duration](abi-testing.html#abi-sys-bus-edac-devices-dev-name-scrubx-max-cycle-duration)
* [/sys/bus/edac/devices/<dev-name>/scrubX/current\_cycle\_duration](abi-testing.html#abi-sys-bus-edac-devices-dev-name-scrubx-current-cycle-duration)

## ABI file testing/sysfs-firmware-acpi

Has the following ABI:

* [/sys/firmware/acpi/fpdt/](abi-testing.html#abi-sys-firmware-acpi-fpdt)
* [/sys/firmware/acpi/bgrt/](abi-testing.html#abi-sys-firmware-acpi-bgrt)
* [/sys/firmware/acpi/hotplug/](abi-testing.html#abi-sys-firmware-acpi-hotplug)
* [/sys/firmware/acpi/interrupts/](abi-testing.html#abi-sys-firmware-acpi-interrupts)
* [/sys/firmware/acpi/memory\_ranges/rangeX](abi-testing.html#abi-sys-firmware-acpi-memory-ranges-rangex)

## ABI file testing/sysfs-firmware-dmi-entries

Has the following ABI:

* [/sys/firmware/dmi/entries/](abi-testing.html#abi-sys-firmware-dmi-entries)

## ABI file testing/sysfs-firmware-dmi-tables

Has the following ABI:

* [/sys/firmware/dmi/tables/](abi-testing.html#abi-sys-firmware-dmi-tables)

## ABI file testing/sysfs-firmware-efi

Has the following ABI:

* [/sys/firmware/efi/fw\_vendor](abi-testing.html#abi-sys-firmware-efi-fw-vendor)
* [/sys/firmware/efi/runtime](abi-testing.html#abi-sys-firmware-efi-runtime)
* [/sys/firmware/efi/config\_table](abi-testing.html#abi-sys-firmware-efi-config-table)
* [/sys/firmware/efi/systab](abi-testing.html#abi-sys-firmware-efi-systab)
* [/sys/firmware/efi/tables/rci2](abi-testing.html#abi-sys-firmware-efi-tables-rci2)
* [/sys/firmware/efi/ovmf\_debug\_log](abi-testing.html#abi-sys-firmware-efi-ovmf-debug-log)

## ABI file testing/sysfs-firmware-efi-esrt

Has the following ABI:

* [/sys/firmware/efi/esrt/](abi-testing.html#abi-sys-firmware-efi-esrt)
* [/sys/firmware/efi/esrt/fw\_resource\_count](abi-testing.html#abi-sys-firmware-efi-esrt-fw-resource-count)
* [/sys/firmware/efi/esrt/fw\_resource\_count\_max](abi-testing.html#abi-sys-firmware-efi-esrt-fw-resource-count-max)
* [/sys/firmware/efi/esrt/fw\_resource\_version](abi-testing.html#abi-sys-firmware-efi-esrt-fw-resource-version)
* [/sys/firmware/efi/esrt/entries/entry<N>/](abi-testing.html#abi-sys-firmware-efi-esrt-entries-entry-n)
* [/sys/firmware/efi/esrt/entries/entry<N>/fw\_type](abi-testing.html#abi-sys-firmware-efi-esrt-entries-entry-n-fw-type)
* [/sys/firmware/efi/esrt/entries/entry<N>/fw\_class](abi-testing.html#abi-sys-firmware-efi-esrt-entries-entry-n-fw-class)
* [/sys/firmware/efi/esrt/entries/entry<N>/fw\_version](abi-testing.html#abi-sys-firmware-efi-esrt-entries-entry-n-fw-version)
* [/sys/firmware/efi/esrt/entries/entry<N>/lowest\_supported\_fw\_version](abi-testing.html#abi-sys-firmware-efi-esrt-entries-entry-n-lowest-supported-fw-version)
* [/sys/firmware/efi/esrt/entries/entry<N>/capsule\_flags](abi-testing.html#abi-sys-firmware-efi-esrt-entries-entry-n-capsule-flags)
* [/sys/firmware/efi/esrt/entries/entry<N>/last\_attempt\_version](abi-testing.html#abi-sys-firmware-efi-esrt-entries-entry-n-last-attempt-version)
* [/sys/firmware/efi/esrt/entries/entry<N>/last\_attempt\_status](abi-testing.html#abi-sys-firmware-efi-esrt-entries-entry-n-last-attempt-status)

## ABI file testing/sysfs-firmware-efi-runtime-map

Has the following ABI:

* [/sys/firmware/efi/runtime-map/](abi-testing.html#abi-sys-firmware-efi-runtime-map)

## ABI file testing/sysfs-firmware-gsmi

Has the following ABI:

* [/sys/firmware/gsmi](abi-testing.html#abi-sys-firmware-gsmi)

## ABI file testing/sysfs-firmware-initrd

Has the following ABI:

* [/sys/firmware/initrd](abi-testing.html#abi-sys-firmware-initrd)

## ABI file testing/sysfs-firmware-lefi-boardinfo

Has the following ABI:

* [/sys/firmware/lefi/boardinfo](abi-testing.html#abi-sys-firmware-lefi-boardinfo)

## ABI file testing/sysfs-firmware-log

Has the following ABI:

* [/sys/firmware/log](abi-testing.html#abi-sys-firmware-log)

## ABI file testing/sysfs-firmware-memmap

Has the following ABI:

* [/sys/firmware/memmap/](abi-testing.html#abi-sys-firmware-memmap)

## ABI file testing/sysfs-firmware-ofw

Has the following ABI:

* [/sys/firmware/devicetree/\*](abi-testing.html#abi-sys-firmware-devicetree)
* [/sys/firmware/fdt](abi-testing.html#abi-sys-firmware-fdt)

## ABI file testing/sysfs-firmware-opal-powercap

Has the following ABI:

* [/sys/firmware/opal/powercap](abi-testing.html#abi-sys-firmware-opal-powercap)
* [/sys/firmware/opal/powercap/system-powercap](abi-testing.html#abi-sys-firmware-opal-powercap-system-powercap)

## ABI file testing/sysfs-firmware-opal-psr

Has the following ABI:

* [/sys/firmware/opal/psr](abi-testing.html#abi-sys-firmware-opal-psr)
* [/sys/firmware/opal/psr/cpu\_to\_gpu\_X](abi-testing.html#abi-sys-firmware-opal-psr-cpu-to-gpu-x)

## ABI file testing/sysfs-firmware-opal-sensor-groups

Has the following ABI:

* [/sys/firmware/opal/sensor\_groups](abi-testing.html#abi-sys-firmware-opal-sensor-groups)
* [/sys/firmware/opal/sensor\_groups/<sensor\_group\_name>/clear](abi-testing.html#abi-sys-firmware-opal-sensor-groups-sensor-group-name-clear)

## ABI file testing/sysfs-firmware-papr-energy-scale-info

Has the following ABI:

* [/sys/firmware/papr/energy\_scale\_info](abi-testing.html#abi-sys-firmware-papr-energy-scale-info)
* [/sys/firmware/papr/energy\_scale\_info/<id>](abi-testing.html#abi-sys-firmware-papr-energy-scale-info-id)
* [/sys/firmware/papr/energy\_scale\_info/<id>/desc](abi-testing.html#abi-sys-firmware-papr-energy-scale-info-id-desc)
* [/sys/firmware/papr/energy\_scale\_info/<id>/value](abi-testing.html#abi-sys-firmware-papr-energy-scale-info-id-value)
* [/sys/firmware/papr/energy\_scale\_info/<id>/value\_desc](abi-testing.html#abi-sys-firmware-papr-energy-scale-info-id-value-desc)

## ABI file testing/sysfs-firmware-plpks

Has the following ABI:

* [/sys/firmware/plpks/config](abi-testing.html#abi-sys-firmware-plpks-config)
* [/sys/firmware/plpks/config/version](abi-testing.html#abi-sys-firmware-plpks-config-version)
* [/sys/firmware/plpks/config/max\_object\_size](abi-testing.html#abi-sys-firmware-plpks-config-max-object-size)
* [/sys/firmware/plpks/config/total\_size](abi-testing.html#abi-sys-firmware-plpks-config-total-size)
* [/sys/firmware/plpks/config/used\_space](abi-testing.html#abi-sys-firmware-plpks-config-used-space)
* [/sys/firmware/plpks/config/supported\_policies](abi-testing.html#abi-sys-firmware-plpks-config-supported-policies)
* [/sys/firmware/plpks/config/signed\_update\_algorithms](abi-testing.html#abi-sys-firmware-plpks-config-signed-update-algorithms)
* [/sys/firmware/plpks/config/wrapping\_features](abi-testing.html#abi-sys-firmware-plpks-config-wrapping-features)

## ABI file testing/sysfs-firmware-qemu\_fw\_cfg

Has the following ABI:

* [/sys/firmware/qemu\_fw\_cfg/](abi-testing.html#abi-sys-firmware-qemu-fw-cfg)

## ABI file testing/sysfs-firmware-sgi\_uv

Has the following ABI:

* [/sys/firmware/sgi\_uv/](abi-testing.html#abi-sys-firmware-sgi-uv)

## ABI file testing/sysfs-firmware-turris-mox-rwtm

Has the following ABI:

* [/sys/firmware/turris-mox-rwtm/board\_version](abi-testing.html#abi-sys-firmware-turris-mox-rwtm-board-version)
* [/sys/firmware/turris-mox-rwtm/mac\_address\*](abi-testing.html#abi-sys-firmware-turris-mox-rwtm-mac-address)
* [/sys/firmware/turris-mox-rwtm/ram\_size](abi-testing.html#abi-sys-firmware-turris-mox-rwtm-ram-size)
* [/sys/firmware/turris-mox-rwtm/serial\_number](abi-testing.html#abi-sys-firmware-turris-mox-rwtm-serial-number)

## ABI file testing/sysfs-fs-erofs

Has the following ABI:

* [/sys/fs/erofs/features/](abi-testing.html#abi-sys-fs-erofs-features)
* [/sys/fs/erofs/<disk>/sync\_decompress](abi-testing.html#abi-sys-fs-erofs-disk-sync-decompress)
* [/sys/fs/erofs/<disk>/drop\_caches](abi-testing.html#abi-sys-fs-erofs-disk-drop-caches)
* [/sys/fs/erofs/accel](abi-testing.html#abi-sys-fs-erofs-accel)
* [/sys/fs/erofs/<disk>/dir\_ra\_bytes](abi-testing.html#abi-sys-fs-erofs-disk-dir-ra-bytes)

## ABI file testing/sysfs-fs-ext4

Has the following ABI:

* [/sys/fs/ext4/<disk>/mb\_stats](abi-testing.html#abi-sys-fs-ext4-disk-mb-stats)
* [/sys/fs/ext4/<disk>/mb\_group\_prealloc](abi-testing.html#abi-sys-fs-ext4-disk-mb-group-prealloc)
* [/sys/fs/ext4/<disk>/mb\_max\_to\_scan](abi-testing.html#abi-sys-fs-ext4-disk-mb-max-to-scan)
* [/sys/fs/ext4/<disk>/mb\_min\_to\_scan](abi-testing.html#abi-sys-fs-ext4-disk-mb-min-to-scan)
* [/sys/fs/ext4/<disk>/mb\_order2\_req](abi-testing.html#abi-sys-fs-ext4-disk-mb-order2-req)
* [/sys/fs/ext4/<disk>/mb\_stream\_req](abi-testing.html#abi-sys-fs-ext4-disk-mb-stream-req)
* [/sys/fs/ext4/<disk>/inode\_readahead\_blks](abi-testing.html#abi-sys-fs-ext4-disk-inode-readahead-blks)
* [/sys/fs/ext4/<disk>/delayed\_allocation\_blocks](abi-testing.html#abi-sys-fs-ext4-disk-delayed-allocation-blocks)
* [/sys/fs/ext4/<disk>/lifetime\_write\_kbytes](abi-testing.html#abi-sys-fs-ext4-disk-lifetime-write-kbytes)
* [/sys/fs/ext4/<disk>/session\_write\_kbytes](abi-testing.html#abi-sys-fs-ext4-disk-session-write-kbytes)
* [/sys/fs/ext4/<disk>/inode\_goal](abi-testing.html#abi-sys-fs-ext4-disk-inode-goal)
* [/sys/fs/ext4/<disk>/max\_writeback\_mb\_bump](abi-testing.html#abi-sys-fs-ext4-disk-max-writeback-mb-bump)
* [/sys/fs/ext4/<disk>/extent\_max\_zeroout\_kb](abi-testing.html#abi-sys-fs-ext4-disk-extent-max-zeroout-kb)
* [/sys/fs/ext4/<disk>/journal\_task](abi-testing.html#abi-sys-fs-ext4-disk-journal-task)

## ABI file testing/sysfs-fs-f2fs

Has the following ABI:

* [/sys/fs/f2fs/<disk>/gc\_max\_sleep\_time](abi-testing.html#abi-sys-fs-f2fs-disk-gc-max-sleep-time)
* [/sys/fs/f2fs/<disk>/gc\_min\_sleep\_time](abi-testing.html#abi-sys-fs-f2fs-disk-gc-min-sleep-time)
* [/sys/fs/f2fs/<disk>/gc\_no\_gc\_sleep\_time](abi-testing.html#abi-sys-fs-f2fs-disk-gc-no-gc-sleep-time)
* [/sys/fs/f2fs/<disk>/gc\_idle](abi-testing.html#abi-sys-fs-f2fs-disk-gc-idle)
* [/sys/fs/f2fs/<disk>/reclaim\_segments](abi-testing.html#abi-sys-fs-f2fs-disk-reclaim-segments)
* [/sys/fs/f2fs/<disk>/main\_blkaddr](abi-testing.html#abi-sys-fs-f2fs-disk-main-blkaddr)
* [/sys/fs/f2fs/<disk>/ipu\_policy](abi-testing.html#abi-sys-fs-f2fs-disk-ipu-policy)
* [/sys/fs/f2fs/<disk>/min\_ipu\_util](abi-testing.html#abi-sys-fs-f2fs-disk-min-ipu-util)
* [/sys/fs/f2fs/<disk>/min\_fsync\_blocks](abi-testing.html#abi-sys-fs-f2fs-disk-min-fsync-blocks)
* [/sys/fs/f2fs/<disk>/min\_seq\_blocks](abi-testing.html#abi-sys-fs-f2fs-disk-min-seq-blocks)
* [/sys/fs/f2fs/<disk>/min\_hot\_blocks](abi-testing.html#abi-sys-fs-f2fs-disk-min-hot-blocks)
* [/sys/fs/f2fs/<disk>/min\_ssr\_sections](abi-testing.html#abi-sys-fs-f2fs-disk-min-ssr-sections)
* [/sys/fs/f2fs/<disk>/max\_small\_discards](abi-testing.html#abi-sys-fs-f2fs-disk-max-small-discards)
* [/sys/fs/f2fs/<disk>/max\_ordered\_discard](abi-testing.html#abi-sys-fs-f2fs-disk-max-ordered-discard)
* [/sys/fs/f2fs/<disk>/max\_discard\_request](abi-testing.html#abi-sys-fs-f2fs-disk-max-discard-request)
* [/sys/fs/f2fs/<disk>/min\_discard\_issue\_time](abi-testing.html#abi-sys-fs-f2fs-disk-min-discard-issue-time)
* [/sys/fs/f2fs/<disk>/mid\_discard\_issue\_time](abi-testing.html#abi-sys-fs-f2fs-disk-mid-discard-issue-time)
* [/sys/fs/f2fs/<disk>/max\_discard\_issue\_time](abi-testing.html#abi-sys-fs-f2fs-disk-max-discard-issue-time)
* [/sys/fs/f2fs/<disk>/discard\_granularity](abi-testing.html#abi-sys-fs-f2fs-disk-discard-granularity)
* [/sys/fs/f2fs/<disk>/umount\_discard\_timeout](abi-testing.html#abi-sys-fs-f2fs-disk-umount-discard-timeout)
* [/sys/fs/f2fs/<disk>/pending\_discard](abi-testing.html#abi-sys-fs-f2fs-disk-pending-discard)
* [/sys/fs/f2fs/<disk>/max\_victim\_search](abi-testing.html#abi-sys-fs-f2fs-disk-max-victim-search)
* [/sys/fs/f2fs/<disk>/migration\_granularity](abi-testing.html#abi-sys-fs-f2fs-disk-migration-granularity)
* [/sys/fs/f2fs/<disk>/dir\_level](abi-testing.html#abi-sys-fs-f2fs-disk-dir-level)
* [/sys/fs/f2fs/<disk>/ram\_thresh](abi-testing.html#abi-sys-fs-f2fs-disk-ram-thresh)
* [/sys/fs/f2fs/<disk>/cp\_interval](abi-testing.html#abi-sys-fs-f2fs-disk-cp-interval)
* [/sys/fs/f2fs/<disk>/idle\_interval](abi-testing.html#abi-sys-fs-f2fs-disk-idle-interval)
* [/sys/fs/f2fs/<disk>/discard\_idle\_interval](abi-testing.html#abi-sys-fs-f2fs-disk-discard-idle-interval)
* [/sys/fs/f2fs/<disk>/gc\_idle\_interval](abi-testing.html#abi-sys-fs-f2fs-disk-gc-idle-interval)
* [/sys/fs/f2fs/<disk>/iostat\_enable](abi-testing.html#abi-sys-fs-f2fs-disk-iostat-enable)
* [/sys/fs/f2fs/<disk>/ra\_nid\_pages](abi-testing.html#abi-sys-fs-f2fs-disk-ra-nid-pages)
* [/sys/fs/f2fs/<disk>/dirty\_nats\_ratio](abi-testing.html#abi-sys-fs-f2fs-disk-dirty-nats-ratio)
* [/sys/fs/f2fs/<disk>/lifetime\_write\_kbytes](abi-testing.html#abi-sys-fs-f2fs-disk-lifetime-write-kbytes)
* [/sys/fs/f2fs/<disk>/features](abi-testing.html#abi-sys-fs-f2fs-disk-features)
* [/sys/fs/f2fs/<disk>/feature\_list/](abi-testing.html#abi-sys-fs-f2fs-disk-feature-list)
* [/sys/fs/f2fs/features/](abi-testing.html#abi-sys-fs-f2fs-features)
* [/sys/fs/f2fs/<disk>/inject\_rate](abi-testing.html#abi-sys-fs-f2fs-disk-inject-rate)
* [/sys/fs/f2fs/<disk>/inject\_type](abi-testing.html#abi-sys-fs-f2fs-disk-inject-type)
* [/sys/fs/f2fs/<disk>/dirty\_segments](abi-testing.html#abi-sys-fs-f2fs-disk-dirty-segments)
* [/sys/fs/f2fs/<disk>/reserved\_blocks](abi-testing.html#abi-sys-fs-f2fs-disk-reserved-blocks)
* [/sys/fs/f2fs/<disk>/current\_reserved\_blocks](abi-testing.html#abi-sys-fs-f2fs-disk-current-reserved-blocks)
* [/sys/fs/f2fs/<disk>/gc\_urgent](abi-testing.html#abi-sys-fs-f2fs-disk-gc-urgent)
* [/sys/fs/f2fs/<disk>/gc\_urgent\_sleep\_time](abi-testing.html#abi-sys-fs-f2fs-disk-gc-urgent-sleep-time)
* [/sys/fs/f2fs/<disk>/readdir\_ra](abi-testing.html#abi-sys-fs-f2fs-disk-readdir-ra)
* [/sys/fs/f2fs/<disk>/gc\_pin\_file\_thresh](abi-testing.html#abi-sys-fs-f2fs-disk-gc-pin-file-thresh)
* [/sys/fs/f2fs/<disk>/extension\_list](abi-testing.html#abi-sys-fs-f2fs-disk-extension-list)
* [/sys/fs/f2fs/<disk>/unusable](abi-testing.html#abi-sys-fs-f2fs-disk-unusable)
* [/sys/fs/f2fs/<disk>/encoding](abi-testing.html#abi-sys-fs-f2fs-disk-encoding)
* [/sys/fs/f2fs/<disk>/free\_segments](abi-testing.html#abi-sys-fs-f2fs-disk-free-segments)
* [/sys/fs/f2fs/<disk>/cp\_foreground\_calls](abi-testing.html#abi-sys-fs-f2fs-disk-cp-foreground-calls)
* [/sys/fs/f2fs/<disk>/cp\_background\_calls](abi-testing.html#abi-sys-fs-f2fs-disk-cp-background-calls)
* [/sys/fs/f2fs/<disk>/gc\_foreground\_calls](abi-testing.html#abi-sys-fs-f2fs-disk-gc-foreground-calls)
* [/sys/fs/f2fs/<disk>/gc\_background\_calls](abi-testing.html#abi-sys-fs-f2fs-disk-gc-background-calls)
* [/sys/fs/f2fs/<disk>/moved\_blocks\_foreground](abi-testing.html#abi-sys-fs-f2fs-disk-moved-blocks-foreground)
* [/sys/fs/f2fs/<disk>/moved\_blocks\_background](abi-testing.html#abi-sys-fs-f2fs-disk-moved-blocks-background)
* [/sys/fs/f2fs/<disk>/avg\_vblocks](abi-testing.html#abi-sys-fs-f2fs-disk-avg-vblocks)
* [/sys/fs/f2fs/<disk>/defrag\_blocks](abi-testing.html#abi-sys-fs-f2fs-disk-defrag-blocks)
* [/sys/fs/f2fs/<disk>/mounted\_time\_sec](abi-testing.html#abi-sys-fs-f2fs-disk-mounted-time-sec)
* [/sys/fs/f2fs/<disk>/data\_io\_flag](abi-testing.html#abi-sys-fs-f2fs-disk-data-io-flag)
* [/sys/fs/f2fs/<disk>/node\_io\_flag](abi-testing.html#abi-sys-fs-f2fs-disk-node-io-flag)
* [/sys/fs/f2fs/<disk>/iostat\_period\_ms](abi-testing.html#abi-sys-fs-f2fs-disk-iostat-period-ms)
* [/sys/fs/f2fs/<disk>/max\_io\_bytes](abi-testing.html#abi-sys-fs-f2fs-disk-max-io-bytes)
* [/sys/fs/f2fs/<disk>/stat/sb\_status](abi-testing.html#abi-sys-fs-f2fs-disk-stat-sb-status)
* [/sys/fs/f2fs/<disk>/stat/cp\_status](abi-testing.html#abi-sys-fs-f2fs-disk-stat-cp-status)
* [/sys/fs/f2fs/<disk>/stat/issued\_discard](abi-testing.html#abi-sys-fs-f2fs-disk-stat-issued-discard)
* [/sys/fs/f2fs/<disk>/stat/queued\_discard](abi-testing.html#abi-sys-fs-f2fs-disk-stat-queued-discard)
* [/sys/fs/f2fs/<disk>/stat/undiscard\_blks](abi-testing.html#abi-sys-fs-f2fs-disk-stat-undiscard-blks)
* [/sys/fs/f2fs/<disk>/ckpt\_thread\_ioprio](abi-testing.html#abi-sys-fs-f2fs-disk-ckpt-thread-ioprio)
* [/sys/fs/f2fs/<disk>/ovp\_segments](abi-testing.html#abi-sys-fs-f2fs-disk-ovp-segments)
* [/sys/fs/f2fs/<disk>/compr\_written\_block](abi-testing.html#abi-sys-fs-f2fs-disk-compr-written-block)
* [/sys/fs/f2fs/<disk>/compr\_saved\_block](abi-testing.html#abi-sys-fs-f2fs-disk-compr-saved-block)
* [/sys/fs/f2fs/<disk>/compr\_new\_inode](abi-testing.html#abi-sys-fs-f2fs-disk-compr-new-inode)
* [/sys/fs/f2fs/<disk>/atgc\_candidate\_ratio](abi-testing.html#abi-sys-fs-f2fs-disk-atgc-candidate-ratio)
* [/sys/fs/f2fs/<disk>/atgc\_candidate\_count](abi-testing.html#abi-sys-fs-f2fs-disk-atgc-candidate-count)
* [/sys/fs/f2fs/<disk>/atgc\_age\_weight](abi-testing.html#abi-sys-fs-f2fs-disk-atgc-age-weight)
* [/sys/fs/f2fs/<disk>/atgc\_age\_threshold](abi-testing.html#abi-sys-fs-f2fs-disk-atgc-age-threshold)
* [/sys/fs/f2fs/<disk>/atgc\_enabled](abi-testing.html#abi-sys-fs-f2fs-disk-atgc-enabled)
* [/sys/fs/f2fs/<disk>/gc\_reclaimed\_segments](abi-testing.html#abi-sys-fs-f2fs-disk-gc-reclaimed-segments)
* [/sys/fs/f2fs/<disk>/gc\_segment\_mode](abi-testing.html#abi-sys-fs-f2fs-disk-gc-segment-mode)
* [/sys/fs/f2fs/<disk>/seq\_file\_ra\_mul](abi-testing.html#abi-sys-fs-f2fs-disk-seq-file-ra-mul)
* [/sys/fs/f2fs/<disk>/max\_fragment\_chunk](abi-testing.html#abi-sys-fs-f2fs-disk-max-fragment-chunk)
* [/sys/fs/f2fs/<disk>/max\_fragment\_hole](abi-testing.html#abi-sys-fs-f2fs-disk-max-fragment-hole)
* [/sys/fs/f2fs/<disk>/gc\_remaining\_trials](abi-testing.html#abi-sys-fs-f2fs-disk-gc-remaining-trials)
* [/sys/fs/f2fs/<disk>/max\_roll\_forward\_node\_blocks](abi-testing.html#abi-sys-fs-f2fs-disk-max-roll-forward-node-blocks)
* [/sys/fs/f2fs/<disk>/unusable\_blocks\_per\_sec](abi-testing.html#abi-sys-fs-f2fs-disk-unusable-blocks-per-sec)
* [/sys/fs/f2fs/<disk>/max\_open\_zones](abi-testing.html#abi-sys-fs-f2fs-disk-max-open-zones)
* [/sys/fs/f2fs/<disk>/current\_atomic\_write](abi-testing.html#abi-sys-fs-f2fs-disk-current-atomic-write)
* [/sys/fs/f2fs/<disk>/peak\_atomic\_write](abi-testing.html#abi-sys-fs-f2fs-disk-peak-atomic-write)
* [/sys/fs/f2fs/<disk>/committed\_atomic\_block](abi-testing.html#abi-sys-fs-f2fs-disk-committed-atomic-block)
* [/sys/fs/f2fs/<disk>/revoked\_atomic\_block](abi-testing.html#abi-sys-fs-f2fs-disk-revoked-atomic-block)
* [/sys/fs/f2fs/<disk>/gc\_mode](abi-testing.html#abi-sys-fs-f2fs-disk-gc-mode)
* [/sys/fs/f2fs/<disk>/discard\_urgent\_util](abi-testing.html#abi-sys-fs-f2fs-disk-discard-urgent-util)
* [/sys/fs/f2fs/<disk>/hot\_data\_age\_threshold](abi-testing.html#abi-sys-fs-f2fs-disk-hot-data-age-threshold)
* [/sys/fs/f2fs/<disk>/warm\_data\_age\_threshold](abi-testing.html#abi-sys-fs-f2fs-disk-warm-data-age-threshold)
* [/sys/fs/f2fs/<disk>/fault\_rate](abi-testing.html#abi-sys-fs-f2fs-disk-fault-rate)
* [/sys/fs/f2fs/<disk>/fault\_type](abi-testing.html#abi-sys-fs-f2fs-disk-fault-type)
* [/sys/fs/f2fs/<disk>/discard\_io\_aware\_gran](abi-testing.html#abi-sys-fs-f2fs-disk-discard-io-aware-gran)
* [/sys/fs/f2fs/<disk>/last\_age\_weight](abi-testing.html#abi-sys-fs-f2fs-disk-last-age-weight)
* [/sys/fs/f2fs/<disk>/compress\_watermark](abi-testing.html#abi-sys-fs-f2fs-disk-compress-watermark)
* [/sys/fs/f2fs/<disk>/compress\_percent](abi-testing.html#abi-sys-fs-f2fs-disk-compress-percent)
* [/sys/fs/f2fs/<disk>/discard\_io\_aware](abi-testing.html#abi-sys-fs-f2fs-disk-discard-io-aware)
* [/sys/fs/f2fs/<disk>/blkzone\_alloc\_policy](abi-testing.html#abi-sys-fs-f2fs-disk-blkzone-alloc-policy)
* [/sys/fs/f2fs/<disk>/migration\_window\_granularity](abi-testing.html#abi-sys-fs-f2fs-disk-migration-window-granularity)
* [/sys/fs/f2fs/<disk>/reserved\_segments](abi-testing.html#abi-sys-fs-f2fs-disk-reserved-segments)
* [/sys/fs/f2fs/<disk>/gc\_no\_zoned\_gc\_percent](abi-testing.html#abi-sys-fs-f2fs-disk-gc-no-zoned-gc-percent)
* [/sys/fs/f2fs/<disk>/gc\_boost\_zoned\_gc\_percent](abi-testing.html#abi-sys-fs-f2fs-disk-gc-boost-zoned-gc-percent)
* [/sys/fs/f2fs/<disk>/gc\_valid\_thresh\_ratio](abi-testing.html#abi-sys-fs-f2fs-disk-gc-valid-thresh-ratio)
* [/sys/fs/f2fs/<disk>/max\_read\_extent\_count](abi-testing.html#abi-sys-fs-f2fs-disk-max-read-extent-count)
* [/sys/fs/f2fs/tuning/reclaim\_caches\_kb](abi-testing.html#abi-sys-fs-f2fs-tuning-reclaim-caches-kb)
* [/sys/fs/f2fs/<disk>/carve\_out](abi-testing.html#abi-sys-fs-f2fs-disk-carve-out)
* [/sys/fs/f2fs/<disk>/encoding\_flags](abi-testing.html#abi-sys-fs-f2fs-disk-encoding-flags)
* [/sys/fs/f2fs/<disk>/reserved\_pin\_section](abi-testing.html#abi-sys-fs-f2fs-disk-reserved-pin-section)
* [/sys/fs/f2fs/<disk>/gc\_boost\_gc\_multiple](abi-testing.html#abi-sys-fs-f2fs-disk-gc-boost-gc-multiple)
* [/sys/fs/f2fs/<disk>/gc\_boost\_gc\_greedy](abi-testing.html#abi-sys-fs-f2fs-disk-gc-boost-gc-greedy)
* [/sys/fs/f2fs/<disk>/effective\_lookup\_mode](abi-testing.html#abi-sys-fs-f2fs-disk-effective-lookup-mode)
* [/sys/fs/f2fs/<disk>/bggc\_io\_aware](abi-testing.html#abi-sys-fs-f2fs-disk-bggc-io-aware)
* [/sys/fs/f2fs/<disk>/allocate\_section\_hint](abi-testing.html#abi-sys-fs-f2fs-disk-allocate-section-hint)
* [/sys/fs/f2fs/<disk>/allocate\_section\_policy](abi-testing.html#abi-sys-fs-f2fs-disk-allocate-section-policy)
* [/sys/fs/f2fs/<disk>/max\_lock\_elapsed\_time](abi-testing.html#abi-sys-fs-f2fs-disk-max-lock-elapsed-time)
* [/sys/fs/f2fs/<disk>/inject\_timeout\_type](abi-testing.html#abi-sys-fs-f2fs-disk-inject-timeout-type)
* [/sys/fs/f2fs/<disk>/adjust\_lock\_priority](abi-testing.html#abi-sys-fs-f2fs-disk-adjust-lock-priority)
* [/sys/fs/f2fs/<disk>/lock\_duration\_priority](abi-testing.html#abi-sys-fs-f2fs-disk-lock-duration-priority)
* [/sys/fs/f2fs/<disk>/critical\_task\_priority](abi-testing.html#abi-sys-fs-f2fs-disk-critical-task-priority)

## ABI file testing/sysfs-fs-nilfs2

Has the following ABI:

* [/sys/fs/nilfs2/features/revision](abi-testing.html#abi-sys-fs-nilfs2-features-revision)
* [/sys/fs/nilfs2/features/README](abi-testing.html#abi-sys-fs-nilfs2-features-readme)
* [/sys/fs/nilfs2/<device>/revision](abi-testing.html#abi-sys-fs-nilfs2-device-revision)
* [/sys/fs/nilfs2/<device>/blocksize](abi-testing.html#abi-sys-fs-nilfs2-device-blocksize)
* [/sys/fs/nilfs2/<device>/device\_size](abi-testing.html#abi-sys-fs-nilfs2-device-device-size)
* [/sys/fs/nilfs2/<device>/free\_blocks](abi-testing.html#abi-sys-fs-nilfs2-device-free-blocks)
* [/sys/fs/nilfs2/<device>/uuid](abi-testing.html#abi-sys-fs-nilfs2-device-uuid)
* [/sys/fs/nilfs2/<device>/volume\_name](abi-testing.html#abi-sys-fs-nilfs2-device-volume-name)
* [/sys/fs/nilfs2/<device>/README](abi-testing.html#abi-sys-fs-nilfs2-device-readme)
* [/sys/fs/nilfs2/<device>/superblock/sb\_write\_time](abi-testing.html#abi-sys-fs-nilfs2-device-superblock-sb-write-time)
* [/sys/fs/nilfs2/<device>/superblock/sb\_write\_time\_secs](abi-testing.html#abi-sys-fs-nilfs2-device-superblock-sb-write-time-secs)
* [/sys/fs/nilfs2/<device>/superblock/sb\_write\_count](abi-testing.html#abi-sys-fs-nilfs2-device-superblock-sb-write-count)
* [/sys/fs/nilfs2/<device>/superblock/sb\_update\_frequency](abi-testing.html#abi-sys-fs-nilfs2-device-superblock-sb-update-frequency)
* [/sys/fs/nilfs2/<device>/superblock/README](abi-testing.html#abi-sys-fs-nilfs2-device-superblock-readme)
* [/sys/fs/nilfs2/<device>/segctor/last\_pseg\_block](abi-testing.html#abi-sys-fs-nilfs2-device-segctor-last-pseg-block)
* [/sys/fs/nilfs2/<device>/segctor/last\_seg\_sequence](abi-testing.html#abi-sys-fs-nilfs2-device-segctor-last-seg-sequence)
* [/sys/fs/nilfs2/<device>/segctor/last\_seg\_checkpoint](abi-testing.html#abi-sys-fs-nilfs2-device-segctor-last-seg-checkpoint)
* [/sys/fs/nilfs2/<device>/segctor/current\_seg\_sequence](abi-testing.html#abi-sys-fs-nilfs2-device-segctor-current-seg-sequence)
* [/sys/fs/nilfs2/<device>/segctor/current\_last\_full\_seg](abi-testing.html#abi-sys-fs-nilfs2-device-segctor-current-last-full-seg)
* [/sys/fs/nilfs2/<device>/segctor/next\_full\_seg](abi-testing.html#abi-sys-fs-nilfs2-device-segctor-next-full-seg)
* [/sys/fs/nilfs2/<device>/segctor/next\_pseg\_offset](abi-testing.html#abi-sys-fs-nilfs2-device-segctor-next-pseg-offset)
* [/sys/fs/nilfs2/<device>/segctor/next\_checkpoint](abi-testing.html#abi-sys-fs-nilfs2-device-segctor-next-checkpoint)
* [/sys/fs/nilfs2/<device>/segctor/last\_seg\_write\_time](abi-testing.html#abi-sys-fs-nilfs2-device-segctor-last-seg-write-time)
* [/sys/fs/nilfs2/<device>/segctor/last\_seg\_write\_time\_secs](abi-testing.html#abi-sys-fs-nilfs2-device-segctor-last-seg-write-time-secs)
* [/sys/fs/nilfs2/<device>/segctor/last\_nongc\_write\_time](abi-testing.html#abi-sys-fs-nilfs2-device-segctor-last-nongc-write-time)
* [/sys/fs/nilfs2/<device>/segctor/last\_nongc\_write\_time\_secs](abi-testing.html#abi-sys-fs-nilfs2-device-segctor-last-nongc-write-time-secs)
* [/sys/fs/nilfs2/<device>/segctor/dirty\_data\_blocks\_count](abi-testing.html#abi-sys-fs-nilfs2-device-segctor-dirty-data-blocks-count)
* [/sys/fs/nilfs2/<device>/segctor/README](abi-testing.html#abi-sys-fs-nilfs2-device-segctor-readme)
* [/sys/fs/nilfs2/<device>/segments/segments\_number](abi-testing.html#abi-sys-fs-nilfs2-device-segments-segments-number)
* [/sys/fs/nilfs2/<device>/segments/blocks\_per\_segment](abi-testing.html#abi-sys-fs-nilfs2-device-segments-blocks-per-segment)
* [/sys/fs/nilfs2/<device>/segments/clean\_segments](abi-testing.html#abi-sys-fs-nilfs2-device-segments-clean-segments)
* [/sys/fs/nilfs2/<device>/segments/dirty\_segments](abi-testing.html#abi-sys-fs-nilfs2-device-segments-dirty-segments)
* [/sys/fs/nilfs2/<device>/segments/README](abi-testing.html#abi-sys-fs-nilfs2-device-segments-readme)
* [/sys/fs/nilfs2/<device>/checkpoints/checkpoints\_number](abi-testing.html#abi-sys-fs-nilfs2-device-checkpoints-checkpoints-number)
* [/sys/fs/nilfs2/<device>/checkpoints/snapshots\_number](abi-testing.html#abi-sys-fs-nilfs2-device-checkpoints-snapshots-number)
* [/sys/fs/nilfs2/<device>/checkpoints/last\_seg\_checkpoint](abi-testing.html#abi-sys-fs-nilfs2-device-checkpoints-last-seg-checkpoint)
* [/sys/fs/nilfs2/<device>/checkpoints/next\_checkpoint](abi-testing.html#abi-sys-fs-nilfs2-device-checkpoints-next-checkpoint)
* [/sys/fs/nilfs2/<device>/checkpoints/README](abi-testing.html#abi-sys-fs-nilfs2-device-checkpoints-readme)
* [/sys/fs/nilfs2/<device>/mounted\_snapshots/README](abi-testing.html#abi-sys-fs-nilfs2-device-mounted-snapshots-readme)
* [/sys/fs/nilfs2/<device>/mounted\_snapshots/<id>/inodes\_count](abi-testing.html#abi-sys-fs-nilfs2-device-mounted-snapshots-id-inodes-count)
* [/sys/fs/nilfs2/<device>/mounted\_snapshots/<id>/blocks\_count](abi-testing.html#abi-sys-fs-nilfs2-device-mounted-snapshots-id-blocks-count)
* [/sys/fs/nilfs2/<device>/mounted\_snapshots/<id>/README](abi-testing.html#abi-sys-fs-nilfs2-device-mounted-snapshots-id-readme)

## ABI file testing/sysfs-fs-ubifs

Has the following ABI:

* [/sys/fs/ubifsX\_Y/error\_magic](abi-testing.html#abi-sys-fs-ubifsx-y-error-magic)
* [/sys/fs/ubifsX\_Y/error\_node](abi-testing.html#abi-sys-fs-ubifsx-y-error-node)
* [/sys/fs/ubifsX\_Y/error\_crc](abi-testing.html#abi-sys-fs-ubifsx-y-error-crc)

## ABI file testing/sysfs-fs-virtiofs

Has the following ABI:

* [/sys/fs/virtiofs/<n>/tag](abi-testing.html#abi-sys-fs-virtiofs-n-tag)
* [/sys/fs/virtiofs/<n>/device](abi-testing.html#abi-sys-fs-virtiofs-n-device)

## ABI file testing/sysfs-fs-xfs

Has the following ABI:

* [/sys/fs/xfs/<disk>/log/log\_head\_lsn](abi-testing.html#abi-sys-fs-xfs-disk-log-log-head-lsn)
* [/sys/fs/xfs/<disk>/log/log\_tail\_lsn](abi-testing.html#abi-sys-fs-xfs-disk-log-log-tail-lsn)
* [/sys/fs/xfs/<disk>/log/reserve\_grant\_head\_bytes](abi-testing.html#abi-sys-fs-xfs-disk-log-reserve-grant-head-bytes)
* [/sys/fs/xfs/<disk>/log/write\_grant\_head\_bytes](abi-testing.html#abi-sys-fs-xfs-disk-log-write-grant-head-bytes)

## ABI file testing/sysfs-hypervisor-xen

Has the following ABI:

* [/sys/hypervisor/guest\_type](abi-testing.html#abi-sys-hypervisor-guest-type)
* [/sys/hypervisor/pmu/pmu\_mode](abi-testing.html#abi-sys-hypervisor-pmu-pmu-mode)
* [/sys/hypervisor/pmu/pmu\_features](abi-testing.html#abi-sys-hypervisor-pmu-pmu-features)
* [/sys/hypervisor/properties/buildid](abi-testing.html#abi-sys-hypervisor-properties-buildid)

## ABI file testing/sysfs-ibft

Has the following ABI:

* [/sys/firmware/ibft/initiator](abi-testing.html#abi-sys-firmware-ibft-initiator)
* [/sys/firmware/ibft/targetX](abi-testing.html#abi-sys-firmware-ibft-targetx)
* [/sys/firmware/ibft/ethernetX](abi-testing.html#abi-sys-firmware-ibft-ethernetx)
* [/sys/firmware/ibft/acpi\_header](abi-testing.html#abi-sys-firmware-ibft-acpi-header)

## ABI file testing/sysfs-kernel-address\_bits

Has the following ABI:

* [/sys/kernel/address\_bits](abi-testing.html#abi-sys-kernel-address-bits)

## ABI file testing/sysfs-kernel-boot\_params

Has the following ABI:

* [/sys/kernel/boot\_params](abi-testing.html#abi-sys-kernel-boot-params)

## ABI file testing/sysfs-kernel-btf

Has the following ABI:

* [/sys/kernel/btf](abi-testing.html#abi-sys-kernel-btf)
* [/sys/kernel/btf/vmlinux](abi-testing.html#abi-sys-kernel-btf-vmlinux)
* [/sys/kernel/btf/<module-name>](abi-testing.html#abi-sys-kernel-btf-module-name)

## ABI file testing/sysfs-kernel-cpu\_byteorder

Has the following ABI:

* [/sys/kernel/cpu\_byteorder](abi-testing.html#abi-sys-kernel-cpu-byteorder)

## ABI file testing/sysfs-kernel-fadump

Has the following ABI:

* [/sys/kernel/fadump/\*](abi-testing.html#abi-sys-kernel-fadump)
* [/sys/kernel/fadump/enabled](abi-testing.html#abi-sys-kernel-fadump-enabledo)
* [/sys/kernel/fadump/registered](abi-testing.html#abi-sys-kernel-fadump-registeredo)
* [/sys/kernel/fadump/release\_mem](abi-testing.html#abi-sys-kernel-fadump-release-memo)
* [/sys/kernel/fadump/mem\_reserved](abi-testing.html#abi-sys-kernel-fadump-mem-reserved)
* [/sys/kernel/fadump/hotplug\_ready](abi-testing.html#abi-sys-kernel-fadump-hotplug-ready)
* [/sys/kernel/fadump/bootargs\_append](abi-testing.html#abi-sys-kernel-fadump-bootargs-append)

## ABI file testing/sysfs-kernel-fscaps

Has the following ABI:

* [/sys/kernel/fscaps](abi-testing.html#abi-sys-kernel-fscaps)

## ABI file testing/sysfs-kernel-hardlockup\_count

Has the following ABI:

* [/sys/kernel/hardlockup\_count](abi-testing.html#abi-sys-kernel-hardlockup-count)

## ABI file testing/sysfs-kernel-iommu\_groups

Has the following ABI:

* [/sys/kernel/iommu\_groups/](abi-testing.html#abi-sys-kernel-iommu-groups)
* [/sys/kernel/iommu\_groups/reserved\_regions](abi-testing.html#abi-sys-kernel-iommu-groups-reserved-regions)
* [/sys/kernel/iommu\_groups/<grp\_id>/type](abi-testing.html#abi-sys-kernel-iommu-groups-grp-id-type)

## ABI file testing/sysfs-kernel-irq

Has the following ABI:

* [/sys/kernel/irq](abi-testing.html#abi-sys-kernel-irq)
* [/sys/kernel/irq/<irq>/actions](abi-testing.html#abi-sys-kernel-irq-irq-actions)
* [/sys/kernel/irq/<irq>/chip\_name](abi-testing.html#abi-sys-kernel-irq-irq-chip-name)
* [/sys/kernel/irq/<irq>/hwirq](abi-testing.html#abi-sys-kernel-irq-irq-hwirq)
* [/sys/kernel/irq/<irq>/name](abi-testing.html#abi-sys-kernel-irq-irq-name)
* [/sys/kernel/irq/<irq>/per\_cpu\_count](abi-testing.html#abi-sys-kernel-irq-irq-per-cpu-count)
* [/sys/kernel/irq/<irq>/type](abi-testing.html#abi-sys-kernel-irq-irq-type)
* [/sys/kernel/irq/<irq>/wakeup](abi-testing.html#abi-sys-kernel-irq-irq-wakeup)

## ABI file testing/sysfs-kernel-kexec-kdump

Has the following ABI:

* [/sys/kernel/kexec/\*](abi-testing.html#abi-sys-kernel-kexec)
* [/sys/kernel/kexec/loaded](abi-testing.html#abi-sys-kernel-kexec-loadedo)
* [/sys/kernel/kexec/crash\_loaded](abi-testing.html#abi-sys-kernel-kexec-crash-loadedo)
* [/sys/kernel/kexec/crash\_size](abi-testing.html#abi-sys-kernel-kexec-crash-sizeo)
* [/sys/kernel/kexec/crash\_elfcorehdr\_size](abi-testing.html#abi-sys-kernel-kexec-crash-elfcorehdr-size)
* [/sys/kernel/kexec/crash\_cma\_ranges](abi-testing.html#abi-sys-kernel-kexec-crash-cma-rangeso)

## ABI file testing/sysfs-kernel-livepatch

Has the following ABI:

* [/sys/kernel/livepatch](abi-testing.html#abi-sys-kernel-livepatch)
* [/sys/kernel/livepatch/<patch>](abi-testing.html#abi-sys-kernel-livepatch-patch)
* [/sys/kernel/livepatch/<patch>/enabled](abi-testing.html#abi-sys-kernel-livepatch-patch-enabled)
* [/sys/kernel/livepatch/<patch>/transition](abi-testing.html#abi-sys-kernel-livepatch-patch-transition)
* [/sys/kernel/livepatch/<patch>/force](abi-testing.html#abi-sys-kernel-livepatch-patch-force)
* [/sys/kernel/livepatch/<patch>/replace](abi-testing.html#abi-sys-kernel-livepatch-patch-replace)
* [/sys/kernel/livepatch/<patch>/stack\_order](abi-testing.html#abi-sys-kernel-livepatch-patch-stack-order)
* [/sys/kernel/livepatch/<patch>/<object>](abi-testing.html#abi-sys-kernel-livepatch-patch-object)
* [/sys/kernel/livepatch/<patch>/<object>/patched](abi-testing.html#abi-sys-kernel-livepatch-patch-object-patched)
* [/sys/kernel/livepatch/<patch>/<object>/<function,sympos>](abi-testing.html#abi-sys-kernel-livepatch-patch-object-function-sympos)

## ABI file testing/sysfs-kernel-mm

Has the following ABI:

* [/sys/kernel/mm](abi-testing.html#abi-sys-kernel-mm)

## ABI file testing/sysfs-kernel-mm-cma

Has the following ABI:

* [/sys/kernel/mm/cma/](abi-testing.html#abi-sys-kernel-mm-cma)
* [/sys/kernel/mm/cma/<cma-heap-name>/alloc\_pages\_success](abi-testing.html#abi-sys-kernel-mm-cma-cma-heap-name-alloc-pages-success)
* [/sys/kernel/mm/cma/<cma-heap-name>/alloc\_pages\_fail](abi-testing.html#abi-sys-kernel-mm-cma-cma-heap-name-alloc-pages-fail)
* [/sys/kernel/mm/cma/<cma-heap-name>/release\_pages\_success](abi-testing.html#abi-sys-kernel-mm-cma-cma-heap-name-release-pages-success)
* [/sys/kernel/mm/cma/<cma-heap-name>/total\_pages](abi-testing.html#abi-sys-kernel-mm-cma-cma-heap-name-total-pages)
* [/sys/kernel/mm/cma/<cma-heap-name>/available\_pages](abi-testing.html#abi-sys-kernel-mm-cma-cma-heap-name-available-pages)

## ABI file testing/sysfs-kernel-mm-damon

Has the following ABI:

* [/sys/kernel/mm/damon/](abi-testing.html#abi-sys-kernel-mm-damon)
* [/sys/kernel/mm/damon/admin/](abi-testing.html#abi-sys-kernel-mm-damon-admin)
* [/sys/kernel/mm/damon/admin/kdamonds/nr\_kdamonds](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-nr-kdamonds)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/state](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-state)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/pid](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-pid)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/refresh\_ms](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-refresh-ms)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/nr\_contexts](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-nr-contexts)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/avail\_operations](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-avail-operations)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/operations](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-operations)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/addr\_unit](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-addr-unit)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/monitoring\_attrs/intervals/sample\_us](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-monitoring-attrs-intervals-sample-us)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/monitoring\_attrs/intervals/aggr\_us](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-monitoring-attrs-intervals-aggr-us)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/monitoring\_attrs/intervals/update\_us](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-monitoring-attrs-intervals-update-us)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/monitoring\_attrs/intervals/intrvals\_goal/access\_bp](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-monitoring-attrs-intervals-intrvals-goal-access-bp)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/monitoring\_attrs/intervals/intrvals\_goal/aggrs](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-monitoring-attrs-intervals-intrvals-goal-aggrs)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/monitoring\_attrs/intervals/intrvals\_goal/min\_sample\_us](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-monitoring-attrs-intervals-intrvals-goal-min-sample-us)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/monitoring\_attrs/intervals/intrvals\_goal/max\_sample\_us](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-monitoring-attrs-intervals-intrvals-goal-max-sample-us)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/monitoring\_attrs/nr\_regions/min](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-monitoring-attrs-nr-regions-min)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/monitoring\_attrs/nr\_regions/max](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-monitoring-attrs-nr-regions-max)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/targets/nr\_targets](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-targets-nr-targets)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/targets/<T>/pid\_target](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-targets-t-pid-target)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/targets/<T>/obsolete\_target](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-targets-t-obsolete-target)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/targets/<T>/regions/nr\_regions](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-targets-t-regions-nr-regions)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/targets/<T>/regions/<R>/start](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-targets-t-regions-r-start)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/targets/<T>/regions/<R>/end](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-targets-t-regions-r-end)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/nr\_schemes](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-nr-schemes)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/action](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-action)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/target\_nid](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-target-nid)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/apply\_interval\_us](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-apply-interval-us)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/access\_pattern/sz/min](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-access-pattern-sz-min)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/access\_pattern/sz/max](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-access-pattern-sz-max)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/access\_pattern/nr\_accesses/min](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-access-pattern-nr-accesses-min)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/access\_pattern/nr\_accesses/max](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-access-pattern-nr-accesses-max)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/access\_pattern/age/min](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-access-pattern-age-min)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/access\_pattern/age/max](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-access-pattern-age-max)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/quotas/ms](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-quotas-ms)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/quotas/bytes](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-quotas-bytes)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/quotas/effective\_bytes](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-quotas-effective-bytes)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/quotas/reset\_interval\_ms](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-quotas-reset-interval-ms)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/quotas/goals/nr\_goals](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-quotas-goals-nr-goals)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/quotas/goals/<G>/target\_metric](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-quotas-goals-g-target-metric)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/quotas/goals/<G>/target\_value](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-quotas-goals-g-target-value)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/quotas/goals/<G>/current\_value](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-quotas-goals-g-current-value)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/quotas/goals/<G>/nid](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-quotas-goals-g-nid)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/quotas/goals/<G>/path](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-quotas-goals-g-path)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/quotas/goal\_tuner](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-quotas-goal-tuner)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/quotas/weights/sz\_permil](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-quotas-weights-sz-permil)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/quotas/weights/nr\_accesses\_permil](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-quotas-weights-nr-accesses-permil)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/quotas/weights/age\_permil](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-quotas-weights-age-permil)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/watermarks/metric](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-watermarks-metric)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/watermarks/interval\_us](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-watermarks-interval-us)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/watermarks/high](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-watermarks-high)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/watermarks/mid](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-watermarks-mid)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/watermarks/low](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-watermarks-low)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/filters/nr\_filters](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-filters-nr-filters)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/filters/<F>/type](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-filters-f-type)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/filters/<F>/memcg\_path](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-filters-f-memcg-path)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/filters/<F>/addr\_start](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-filters-f-addr-start)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/filters/<F>/addr\_end](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-filters-f-addr-end)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/filters/<F>/min](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-filters-f-min)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/filters/<F>/max](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-filters-f-max)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/filters/<F>/target\_idx](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-filters-f-target-idx)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/filters/<F>/matching](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-filters-f-matching)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/filters/<F>/allow](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-filters-f-allow)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/core\_filters](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-core-filters)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/ops\_filters](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-ops-filters)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/dests/nr\_dests](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-dests-nr-dests)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/dests/<D>/id](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-dests-d-id)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/dests/<D>/weight](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-dests-d-weight)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/stats/nr\_tried](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-stats-nr-tried)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/stats/sz\_tried](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-stats-sz-tried)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/stats/nr\_applied](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-stats-nr-applied)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/stats/sz\_applied](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-stats-sz-applied)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/stats/sz\_ops\_filter\_passed](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-stats-sz-ops-filter-passed)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/stats/qt\_exceeds](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-stats-qt-exceeds)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/stats/nr\_snapshots](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-stats-nr-snapshots)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/stats/max\_nr\_snapshots](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-stats-max-nr-snapshots)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/tried\_regions/total\_bytes](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-tried-regions-total-bytes)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/tried\_regions/<R>/start](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-tried-regions-r-start)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/tried\_regions/<R>/end](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-tried-regions-r-end)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/tried\_regions/<R>/nr\_accesses](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-tried-regions-r-nr-accesses)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/tried\_regions/<R>/age](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-tried-regions-r-age)
* [/sys/kernel/mm/damon/admin/kdamonds/<K>/contexts/<C>/schemes/<S>/tried\_regions/<R>/sz\_filter\_passed](abi-testing.html#abi-sys-kernel-mm-damon-admin-kdamonds-k-contexts-c-schemes-s-tried-regions-r-sz-filter-passed)

## ABI file testing/sysfs-kernel-mm-hugepages

Has the following ABI:

* [/sys/kernel/mm/hugepages/](abi-testing.html#abi-sys-kernel-mm-hugepages)

## ABI file testing/sysfs-kernel-mm-ksm

Has the following ABI:

* [/sys/kernel/mm/ksm](abi-testing.html#abi-sys-kernel-mm-ksm)
* [/sys/kernel/mm/ksm/full\_scans](abi-testing.html#abi-sys-kernel-mm-ksm-full-scans)
* [/sys/kernel/mm/ksm/pages\_shared](abi-testing.html#abi-sys-kernel-mm-ksm-full-scans)
* [/sys/kernel/mm/ksm/pages\_sharing](abi-testing.html#abi-sys-kernel-mm-ksm-full-scans)
* [/sys/kernel/mm/ksm/pages\_to\_scan](abi-testing.html#abi-sys-kernel-mm-ksm-full-scans)
* [/sys/kernel/mm/ksm/pages\_unshared](abi-testing.html#abi-sys-kernel-mm-ksm-full-scans)
* [/sys/kernel/mm/ksm/pages\_volatile](abi-testing.html#abi-sys-kernel-mm-ksm-full-scans)
* [/sys/kernel/mm/ksm/run](abi-testing.html#abi-sys-kernel-mm-ksm-full-scans)
* [/sys/kernel/mm/ksm/sleep\_millisecs](abi-testing.html#abi-sys-kernel-mm-ksm-full-scans)
* [/sys/kernel/mm/ksm/merge\_across\_nodes](abi-testing.html#abi-sys-kernel-mm-ksm-merge-across-nodes)
* [/sys/kernel/mm/ksm/general\_profit](abi-testing.html#abi-sys-kernel-mm-ksm-general-profit)

## ABI file testing/sysfs-kernel-mm-memory-tiers

Has the following ABI:

* [/sys/devices/virtual/memory\_tiering/](abi-testing.html#abi-sys-devices-virtual-memory-tiering)
* [/sys/devices/virtual/memory\_tiering/memory\_tierN/](abi-testing.html#abi-sys-devices-virtual-memory-tiering-memory-tiern)

## ABI file testing/sysfs-kernel-mm-mempolicy

Has the following ABI:

* [/sys/kernel/mm/mempolicy/](abi-testing.html#abi-sys-kernel-mm-mempolicy)

## ABI file testing/sysfs-kernel-mm-mempolicy-weighted-interleave

Has the following ABI:

* [/sys/kernel/mm/mempolicy/weighted\_interleave/](abi-testing.html#abi-sys-kernel-mm-mempolicy-weighted-interleave)
* [/sys/kernel/mm/mempolicy/weighted\_interleave/nodeN](abi-testing.html#abi-sys-kernel-mm-mempolicy-weighted-interleave-noden)
* [/sys/kernel/mm/mempolicy/weighted\_interleave/auto](abi-testing.html#abi-sys-kernel-mm-mempolicy-weighted-interleave-auto)

## ABI file testing/sysfs-kernel-mm-numa

Has the following ABI:

* [/sys/kernel/mm/numa/](abi-testing.html#abi-sys-kernel-mm-numa)
* [/sys/kernel/mm/numa/demotion\_enabled](abi-testing.html#abi-sys-kernel-mm-numa-demotion-enabled)

## ABI file testing/sysfs-kernel-mm-swap

Has the following ABI:

* [/sys/kernel/mm/swap/](abi-testing.html#abi-sys-kernel-mm-swap)
* [/sys/kernel/mm/swap/vma\_ra\_enabled](abi-testing.html#abi-sys-kernel-mm-swap-vma-ra-enabled)

## ABI file testing/sysfs-kernel-mm-transparent-hugepage

Has the following ABI:

* [/sys/kernel/mm/transparent\_hugepage/](abi-testing.html#abi-sys-kernel-mm-transparent-hugepage)

## ABI file testing/sysfs-kernel-oops\_count

Has the following ABI:

* [/sys/kernel/oops\_count](abi-testing.html#abi-sys-kernel-oops-count)

## ABI file testing/sysfs-kernel-rcu\_stall\_count

Has the following ABI:

* [/sys/kernel/rcu\_stall\_count](abi-testing.html#abi-sys-kernel-rcu-stall-count)

## ABI file testing/sysfs-kernel-reboot

Has the following ABI:

* [/sys/kernel/reboot](abi-testing.html#abi-sys-kernel-reboot)
* [/sys/kernel/reboot/mode](abi-testing.html#abi-sys-kernel-reboot-mode)
* [/sys/kernel/reboot/type](abi-testing.html#abi-sys-kernel-reboot-type)
* [/sys/kernel/reboot/cpu](abi-testing.html#abi-sys-kernel-reboot-cpu)
* [/sys/kernel/reboot/force](abi-testing.html#abi-sys-kernel-reboot-force)
* [/sys/kernel/reboot/hw\_protection](abi-testing.html#abi-sys-kernel-reboot-hw-protection)

## ABI file testing/sysfs-kernel-slab

Has the following ABI:

* [/sys/kernel/slab](abi-testing.html#abi-sys-kernel-slab)
* [/sys/kernel/slab/<cache>/aliases](abi-testing.html#abi-sys-kernel-slab-cache-aliases)
* [/sys/kernel/slab/<cache>/align](abi-testing.html#abi-sys-kernel-slab-cache-align)
* [/sys/kernel/slab/<cache>/alloc\_calls](abi-testing.html#abi-sys-kernel-slab-cache-alloc-calls)
* [/sys/kernel/slab/<cache>/alloc\_fastpath](abi-testing.html#abi-sys-kernel-slab-cache-alloc-fastpath)
* [/sys/kernel/slab/<cache>/alloc\_from\_partial](abi-testing.html#abi-sys-kernel-slab-cache-alloc-from-partial)
* [/sys/kernel/slab/<cache>/alloc\_refill](abi-testing.html#abi-sys-kernel-slab-cache-alloc-refill)
* [/sys/kernel/slab/<cache>/alloc\_slab](abi-testing.html#abi-sys-kernel-slab-cache-alloc-slab)
* [/sys/kernel/slab/<cache>/alloc\_slowpath](abi-testing.html#abi-sys-kernel-slab-cache-alloc-slowpath)
* [/sys/kernel/slab/<cache>/cache\_dma](abi-testing.html#abi-sys-kernel-slab-cache-cache-dma)
* [/sys/kernel/slab/<cache>/cpu\_slabs](abi-testing.html#abi-sys-kernel-slab-cache-cpu-slabs)
* [/sys/kernel/slab/<cache>/cpuslab\_flush](abi-testing.html#abi-sys-kernel-slab-cache-cpuslab-flush)
* [/sys/kernel/slab/<cache>/ctor](abi-testing.html#abi-sys-kernel-slab-cache-ctor)
* [/sys/kernel/slab/<cache>/deactivate\_empty](abi-testing.html#abi-sys-kernel-slab-cache-deactivate-empty)
* [/sys/kernel/slab/<cache>/deactivate\_full](abi-testing.html#abi-sys-kernel-slab-cache-deactivate-full)
* [/sys/kernel/slab/<cache>/deactivate\_remote\_frees](abi-testing.html#abi-sys-kernel-slab-cache-deactivate-remote-frees)
* [/sys/kernel/slab/<cache>/deactivate\_to\_head](abi-testing.html#abi-sys-kernel-slab-cache-deactivate-to-head)
* [/sys/kernel/slab/<cache>/deactivate\_to\_tail](abi-testing.html#abi-sys-kernel-slab-cache-deactivate-to-tail)
* [/sys/kernel/slab/<cache>/destroy\_by\_rcu](abi-testing.html#abi-sys-kernel-slab-cache-destroy-by-rcu)
* [/sys/kernel/slab/<cache>/free\_add\_partial](abi-testing.html#abi-sys-kernel-slab-cache-free-add-partial)
* [/sys/kernel/slab/<cache>/free\_calls](abi-testing.html#abi-sys-kernel-slab-cache-free-calls)
* [/sys/kernel/slab/<cache>/free\_fastpath](abi-testing.html#abi-sys-kernel-slab-cache-free-fastpath)
* [/sys/kernel/slab/<cache>/free\_frozen](abi-testing.html#abi-sys-kernel-slab-cache-free-frozen)
* [/sys/kernel/slab/<cache>/free\_remove\_partial](abi-testing.html#abi-sys-kernel-slab-cache-free-remove-partial)
* [/sys/kernel/slab/<cache>/free\_slab](abi-testing.html#abi-sys-kernel-slab-cache-free-slab)
* [/sys/kernel/slab/<cache>/free\_slowpath](abi-testing.html#abi-sys-kernel-slab-cache-free-slowpath)
* [/sys/kernel/slab/<cache>/hwcache\_align](abi-testing.html#abi-sys-kernel-slab-cache-hwcache-align)
* [/sys/kernel/slab/<cache>/min\_partial](abi-testing.html#abi-sys-kernel-slab-cache-min-partial)
* [/sys/kernel/slab/<cache>/object\_size](abi-testing.html#abi-sys-kernel-slab-cache-object-size)
* [/sys/kernel/slab/<cache>/objects](abi-testing.html#abi-sys-kernel-slab-cache-objects)
* [/sys/kernel/slab/<cache>/objects\_partial](abi-testing.html#abi-sys-kernel-slab-cache-objects-partial)
* [/sys/kernel/slab/<cache>/objs\_per\_slab](abi-testing.html#abi-sys-kernel-slab-cache-objs-per-slab)
* [/sys/kernel/slab/<cache>/order](abi-testing.html#abi-sys-kernel-slab-cache-order)
* [/sys/kernel/slab/<cache>/order\_fallback](abi-testing.html#abi-sys-kernel-slab-cache-order-fallback)
* [/sys/kernel/slab/<cache>/partial](abi-testing.html#abi-sys-kernel-slab-cache-partial)
* [/sys/kernel/slab/<cache>/poison](abi-testing.html#abi-sys-kernel-slab-cache-poison)
* [/sys/kernel/slab/<cache>/reclaim\_account](abi-testing.html#abi-sys-kernel-slab-cache-reclaim-account)
* [/sys/kernel/slab/<cache>/red\_zone](abi-testing.html#abi-sys-kernel-slab-cache-red-zone)
* [/sys/kernel/slab/<cache>/remote\_node\_defrag\_ratio](abi-testing.html#abi-sys-kernel-slab-cache-remote-node-defrag-ratio)
* [/sys/kernel/slab/<cache>/sanity\_checks](abi-testing.html#abi-sys-kernel-slab-cache-sanity-checks)
* [/sys/kernel/slab/<cache>/shrink](abi-testing.html#abi-sys-kernel-slab-cache-shrink)
* [/sys/kernel/slab/<cache>/slab\_size](abi-testing.html#abi-sys-kernel-slab-cache-slab-size)
* [/sys/kernel/slab/<cache>/slabs](abi-testing.html#abi-sys-kernel-slab-cache-slabs)
* [/sys/kernel/slab/<cache>/store\_user](abi-testing.html#abi-sys-kernel-slab-cache-store-user)
* [/sys/kernel/slab/<cache>/total\_objects](abi-testing.html#abi-sys-kernel-slab-cache-total-objects)
* [/sys/kernel/slab/<cache>/trace](abi-testing.html#abi-sys-kernel-slab-cache-trace)
* [/sys/kernel/slab/<cache>/validate](abi-testing.html#abi-sys-kernel-slab-cache-validate)
* [/sys/kernel/slab/<cache>/usersize](abi-testing.html#abi-sys-kernel-slab-cache-usersize)
* [/sys/kernel/slab/<cache>/slabs\_cpu\_partial](abi-testing.html#abi-sys-kernel-slab-cache-slabs-cpu-partial)
* [/sys/kernel/slab/<cache>/cpu\_partial](abi-testing.html#abi-sys-kernel-slab-cache-cpu-partial)

## ABI file testing/sysfs-kernel-softlockup\_count

Has the following ABI:

* [/sys/kernel/softlockup\_count](abi-testing.html#abi-sys-kernel-softlockup-count)

## ABI file testing/sysfs-kernel-vmcoreinfo

Has the following ABI:

* [/sys/kernel/vmcoreinfo](abi-testing.html#abi-sys-kernel-vmcoreinfo)

## ABI file testing/sysfs-kernel-warn\_count

Has the following ABI:

* [/sys/kernel/warn\_count](abi-testing.html#abi-sys-kernel-warn-count)

## ABI file testing/sysfs-mce

Has the following ABI:

* [/sys/devices/system/machinecheck/machinecheckX/](abi-testing.html#abi-sys-devices-system-machinecheck-machinecheckx)
* [/sys/devices/system/machinecheck/machinecheckX/bank<Y>](abi-testing.html#abi-sys-devices-system-machinecheck-machinecheckx-bank-y)
* [/sys/devices/system/machinecheck/machinecheckX/check\_interval](abi-testing.html#abi-sys-devices-system-machinecheck-machinecheckx-check-interval)
* [/sys/devices/system/machinecheck/machinecheckX/trigger](abi-testing.html#abi-sys-devices-system-machinecheck-machinecheckx-trigger)
* [/sys/devices/system/machinecheck/machinecheckX/monarch\_timeout](abi-testing.html#abi-sys-devices-system-machinecheck-machinecheckx-monarch-timeout)
* [/sys/devices/system/machinecheck/machinecheckX/ignore\_ce](abi-testing.html#abi-sys-devices-system-machinecheck-machinecheckx-ignore-ce)
* [/sys/devices/system/machinecheck/machinecheckX/dont\_log\_ce](abi-testing.html#abi-sys-devices-system-machinecheck-machinecheckx-dont-log-ce)
* [/sys/devices/system/machinecheck/machinecheckX/cmci\_disabled](abi-testing.html#abi-sys-devices-system-machinecheck-machinecheckx-cmci-disabled)

## ABI file testing/sysfs-memory-page-offline

Has the following ABI:

* [/sys/devices/system/memory/soft\_offline\_page](abi-testing.html#abi-sys-devices-system-memory-soft-offline-page)
* [/sys/devices/system/memory/hard\_offline\_page](abi-testing.html#abi-sys-devices-system-memory-hard-offline-page)

## ABI file testing/sysfs-module

Has the following ABI:

* [/sys/module/pch\_phub/drivers/.../pch\_mac](abi-testing.html#abi-sys-module-pch-phub-drivers-pch-mac)
* [/sys/module/pch\_phub/drivers/.../pch\_firmware](abi-testing.html#abi-sys-module-pch-phub-drivers-pch-firmware)
* [/sys/module/ehci\_hcd/drivers/.../uframe\_periodic\_max](abi-testing.html#abi-sys-module-ehci-hcd-drivers-uframe-periodic-max)
* [/sys/module/\*/{coresize,initsize}](abi-testing.html#abi-sys-module-coresize-initsize)
* [/sys/module/\*/initstate](abi-testing.html#abi-sys-module-initstate)
* [/sys/module/\*/import\_ns](abi-testing.html#abi-sys-module-import-ns)
* [/sys/module/\*/taint](abi-testing.html#abi-sys-module-taint)
* [/sys/module/grant\_table/parameters/free\_per\_iteration](abi-testing.html#abi-sys-module-grant-table-parameters-free-per-iteration)

## ABI file testing/sysfs-nvme

Has the following ABI:

* [/sys/devices/virtual/nvme-fabrics/ctl/.../tls\_configured\_key](abi-testing.html#abi-sys-devices-virtual-nvme-fabrics-ctl-tls-configured-key)

## ABI file testing/sysfs-nvmem-cells

Has the following ABI:

* [/sys/bus/nvmem/devices/.../cells/<cell-name>](abi-testing.html#abi-sys-bus-nvmem-devices-cells-cell-name)

## ABI file testing/sysfs-ocfs2

Has the following ABI:

* [/sys/fs/ocfs2/](abi-testing.html#abi-sys-fs-ocfs2)
* [/sys/fs/ocfs2/max\_locking\_protocol](abi-testing.html#abi-sys-fs-ocfs2-max-locking-protocol)
* [/sys/fs/ocfs2/loaded\_cluster\_plugins](abi-testing.html#abi-sys-fs-ocfs2-loaded-cluster-plugins)
* [/sys/fs/ocfs2/active\_cluster\_plugin](abi-testing.html#abi-sys-fs-ocfs2-active-cluster-plugin)
* [/sys/fs/ocfs2/cluster\_stack](abi-testing.html#abi-sys-fs-ocfs2-cluster-stack)

## ABI file testing/sysfs-platform-alienware-wmi

Has the following ABI:

* [/sys/class/hwmon/hwmonX/fanY\_boost](abi-testing.html#abi-sys-class-hwmon-hwmonx-fany-boost)

## ABI file testing/sysfs-platform-asus-laptop

Has the following ABI:

* [/sys/devices/platform/asus\_laptop/display](abi-testing.html#abi-sys-devices-platform-asus-laptop-display)
* [/sys/devices/platform/asus\_laptop/gps](abi-testing.html#abi-sys-devices-platform-asus-laptop-gps)
* [/sys/devices/platform/asus\_laptop/ledd](abi-testing.html#abi-sys-devices-platform-asus-laptop-ledd)
* [/sys/devices/platform/asus\_laptop/bluetooth](abi-testing.html#abi-sys-devices-platform-asus-laptop-bluetooth)
* [/sys/devices/platform/asus\_laptop/wlan](abi-testing.html#abi-sys-devices-platform-asus-laptop-wlan)
* [/sys/devices/platform/asus\_laptop/wimax](abi-testing.html#abi-sys-devices-platform-asus-laptop-wimax)
* [/sys/devices/platform/asus\_laptop/wwan](abi-testing.html#abi-sys-devices-platform-asus-laptop-wwan)

## ABI file testing/sysfs-platform-asus-wmi

Has the following ABI:

* [/sys/devices/platform/<platform>/cpufv](abi-testing.html#abi-sys-devices-platform-platform-cpufv)
* [/sys/devices/platform/<platform>/camera](abi-testing.html#abi-sys-devices-platform-platform-camera)
* [/sys/devices/platform/<platform>/cardr](abi-testing.html#abi-sys-devices-platform-platform-cardr)
* [/sys/devices/platform/<platform>/touchpad](abi-testing.html#abi-sys-devices-platform-platform-touchpad)
* [/sys/devices/platform/<platform>/lid\_resume](abi-testing.html#abi-sys-devices-platform-platform-lid-resume)
* [/sys/devices/platform/<platform>/fan\_boost\_mode](abi-testing.html#abi-sys-devices-platform-platform-fan-boost-mode)
* [/sys/devices/platform/<platform>/throttle\_thermal\_policy](abi-testing.html#abi-sys-devices-platform-platform-throttle-thermal-policy)
* [/sys/devices/platform/<platform>/gpu\_mux\_mode](abi-testing.html#abi-sys-devices-platform-platform-gpu-mux-mode)
* [/sys/devices/platform/<platform>/dgpu\_disable](abi-testing.html#abi-sys-devices-platform-platform-dgpu-disable)
* [/sys/devices/platform/<platform>/egpu\_enable](abi-testing.html#abi-sys-devices-platform-platform-egpu-enable)
* [/sys/devices/platform/<platform>/panel\_od](abi-testing.html#abi-sys-devices-platform-platform-panel-od)
* [/sys/devices/platform/<platform>/charge\_mode](abi-testing.html#abi-sys-devices-platform-platform-charge-mode)
* [/sys/devices/platform/<platform>/egpu\_connected](abi-testing.html#abi-sys-devices-platform-platform-egpu-connected)
* [/sys/devices/platform/<platform>/mini\_led\_mode](abi-testing.html#abi-sys-devices-platform-platform-mini-led-mode)
* [/sys/devices/platform/<platform>/available\_mini\_led\_mode](abi-testing.html#abi-sys-devices-platform-platform-available-mini-led-mode)
* [/sys/devices/platform/<platform>/ppt\_pl1\_spl](abi-testing.html#abi-sys-devices-platform-platform-ppt-pl1-spl)
* [/sys/devices/platform/<platform>/ppt\_pl2\_sppt](abi-testing.html#abi-sys-devices-platform-platform-ppt-pl2-sppt)
* [/sys/devices/platform/<platform>/ppt\_fppt](abi-testing.html#abi-sys-devices-platform-platform-ppt-fppt)
* [/sys/devices/platform/<platform>/ppt\_apu\_sppt](abi-testing.html#abi-sys-devices-platform-platform-ppt-apu-sppt)
* [/sys/devices/platform/<platform>/ppt\_platform\_sppt](abi-testing.html#abi-sys-devices-platform-platform-ppt-platform-sppt)
* [/sys/devices/platform/<platform>/nv\_dynamic\_boost](abi-testing.html#abi-sys-devices-platform-platform-nv-dynamic-boost)
* [/sys/devices/platform/<platform>/nv\_temp\_target](abi-testing.html#abi-sys-devices-platform-platform-nv-temp-target)
* [/sys/devices/platform/<platform>/boot\_sound](abi-testing.html#abi-sys-devices-platform-platform-boot-sound)
* [/sys/devices/platform/<platform>/mcu\_powersave](abi-testing.html#abi-sys-devices-platform-platform-mcu-powersave)

## ABI file testing/sysfs-platform-at91

Has the following ABI:

* [/sys/devices/platform/at91\_can/net/<iface>/mb0\_id](abi-testing.html#abi-sys-devices-platform-at91-can-net-iface-mb0-id)

## ABI file testing/sysfs-platform-ayaneo-ec

Has the following ABI:

* [/sys/devices/platform/ayaneo-ec/controller\_power](abi-testing.html#abi-sys-devices-platform-ayaneo-ec-controller-power)
* [/sys/devices/platform/ayaneo-ec/controller\_modules](abi-testing.html#abi-sys-devices-platform-ayaneo-ec-controller-modules)

## ABI file testing/sysfs-platform-brcmstb-gisb-arb

Has the following ABI:

* [/sys/devices/../../gisb\_arb\_timeout](abi-testing.html#abi-sys-devices-gisb-arb-timeout)

## ABI file testing/sysfs-platform-brcmstb-memc

Has the following ABI:

* [/sys/bus/platform/devices/\*/srpd](abi-testing.html#abi-sys-bus-platform-devices-srpd)
* [/sys/bus/platform/devices/\*/frequency](abi-testing.html#abi-sys-bus-platform-devices-frequency)

## ABI file testing/sysfs-platform-chipidea-usb-otg

Has the following ABI:

* [/sys/bus/platform/devices/ci\_hdrc.0/inputs/a\_bus\_req](abi-testing.html#abi-sys-bus-platform-devices-ci-hdrc-0-inputs-a-bus-req)
* [/sys/bus/platform/devices/ci\_hdrc.0/inputs/a\_bus\_drop](abi-testing.html#abi-sys-bus-platform-devices-ci-hdrc-0-inputs-a-bus-drop)
* [/sys/bus/platform/devices/ci\_hdrc.0/inputs/b\_bus\_req](abi-testing.html#abi-sys-bus-platform-devices-ci-hdrc-0-inputs-b-bus-req)
* [/sys/bus/platform/devices/ci\_hdrc.0/inputs/a\_clr\_err](abi-testing.html#abi-sys-bus-platform-devices-ci-hdrc-0-inputs-a-clr-err)

## ABI file testing/sysfs-platform-chipidea-usb2

Has the following ABI:

* [/sys/bus/platform/devices/ci\_hdrc.0/role](abi-testing.html#abi-sys-bus-platform-devices-ci-hdrc-0-role)

## ABI file testing/sysfs-platform-dell-laptop

Has the following ABI:

* [/sys/class/leds/dell::kbd\_backlight/als\_enabled](abi-testing.html#abi-sys-class-leds-dell-kbd-backlight-als-enabled)
* [/sys/class/leds/dell::kbd\_backlight/als\_setting](abi-testing.html#abi-sys-class-leds-dell-kbd-backlight-als-setting)
* [/sys/class/leds/dell::kbd\_backlight/start\_triggers](abi-testing.html#abi-sys-class-leds-dell-kbd-backlight-start-triggers)
* [/sys/class/leds/dell::kbd\_backlight/stop\_timeout](abi-testing.html#abi-sys-class-leds-dell-kbd-backlight-stop-timeout)

## ABI file testing/sysfs-platform-dell-privacy-wmi

Has the following ABI:

* [/sys/bus/wmi/devices/6932965F-1671-4CEB-B988-D3AB0A901919[-X]/dell\_privacy\_supported\_type](abi-testing.html#abi-sys-bus-wmi-devices-6932965f-1671-4ceb-b988-d3ab0a901919-x-dell-privacy-supported-type)
* [/sys/bus/wmi/devices/6932965F-1671-4CEB-B988-D3AB0A901919[-X]/dell\_privacy\_current\_state](abi-testing.html#abi-sys-bus-wmi-devices-6932965f-1671-4ceb-b988-d3ab0a901919-x-dell-privacy-current-state)

## ABI file testing/sysfs-platform-dell-smbios

Has the following ABI:

* [/sys/devices/platform/<platform>/tokens/\*](abi-testing.html#abi-sys-devices-platform-platform-tokens)

## ABI file testing/sysfs-platform-dell-wmi-ddv

Has the following ABI:

* [/sys/class/power\_supply/<battery\_name>/eppid](abi-testing.html#abi-sys-class-power-supply-battery-name-eppid)

## ABI file testing/sysfs-platform-dfl-fme

Has the following ABI:

* [/sys/bus/platform/devices/dfl-fme.0/ports\_num](abi-testing.html#abi-sys-bus-platform-devices-dfl-fme-0-ports-num)
* [/sys/bus/platform/devices/dfl-fme.0/bitstream\_id](abi-testing.html#abi-sys-bus-platform-devices-dfl-fme-0-bitstream-id)
* [/sys/bus/platform/devices/dfl-fme.0/bitstream\_metadata](abi-testing.html#abi-sys-bus-platform-devices-dfl-fme-0-bitstream-metadata)
* [/sys/bus/platform/devices/dfl-fme.0/cache\_size](abi-testing.html#abi-sys-bus-platform-devices-dfl-fme-0-cache-size)
* [/sys/bus/platform/devices/dfl-fme.0/fabric\_version](abi-testing.html#abi-sys-bus-platform-devices-dfl-fme-0-fabric-version)
* [/sys/bus/platform/devices/dfl-fme.0/socket\_id](abi-testing.html#abi-sys-bus-platform-devices-dfl-fme-0-socket-id)
* [/sys/bus/platform/devices/dfl-fme.0/errors/pcie0\_errors](abi-testing.html#abi-sys-bus-platform-devices-dfl-fme-0-errors-pcie0-errors)
* [/sys/bus/platform/devices/dfl-fme.0/errors/pcie1\_errors](abi-testing.html#abi-sys-bus-platform-devices-dfl-fme-0-errors-pcie1-errors)
* [/sys/bus/platform/devices/dfl-fme.0/errors/nonfatal\_errors](abi-testing.html#abi-sys-bus-platform-devices-dfl-fme-0-errors-nonfatal-errors)
* [/sys/bus/platform/devices/dfl-fme.0/errors/catfatal\_errors](abi-testing.html#abi-sys-bus-platform-devices-dfl-fme-0-errors-catfatal-errors)
* [/sys/bus/platform/devices/dfl-fme.0/errors/inject\_errors](abi-testing.html#abi-sys-bus-platform-devices-dfl-fme-0-errors-inject-errors)
* [/sys/bus/platform/devices/dfl-fme.0/errors/fme\_errors](abi-testing.html#abi-sys-bus-platform-devices-dfl-fme-0-errors-fme-errors)
* [/sys/bus/platform/devices/dfl-fme.0/errors/first\_error](abi-testing.html#abi-sys-bus-platform-devices-dfl-fme-0-errors-first-error)
* [/sys/bus/platform/devices/dfl-fme.0/errors/next\_error](abi-testing.html#abi-sys-bus-platform-devices-dfl-fme-0-errors-next-error)
* [/sys/bus/platform/devices/dfl-fme.0/hwmon/hwmonX/name](abi-testing.html#abi-sys-bus-platform-devices-dfl-fme-0-hwmon-hwmonx-name)
* [/sys/bus/platform/devices/dfl-fme.0/hwmon/hwmonX/temp1\_input](abi-testing.html#abi-sys-bus-platform-devices-dfl-fme-0-hwmon-hwmonx-temp1-input)
* [/sys/bus/platform/devices/dfl-fme.0/hwmon/hwmonX/temp1\_max](abi-testing.html#abi-sys-bus-platform-devices-dfl-fme-0-hwmon-hwmonx-temp1-max)
* [/sys/bus/platform/devices/dfl-fme.0/hwmon/hwmonX/temp1\_crit](abi-testing.html#abi-sys-bus-platform-devices-dfl-fme-0-hwmon-hwmonx-temp1-crit)
* [/sys/bus/platform/devices/dfl-fme.0/hwmon/hwmonX/temp1\_emergency](abi-testing.html#abi-sys-bus-platform-devices-dfl-fme-0-hwmon-hwmonx-temp1-emergency)
* [/sys/bus/platform/devices/dfl-fme.0/hwmon/hwmonX/temp1\_max\_alarm](abi-testing.html#abi-sys-bus-platform-devices-dfl-fme-0-hwmon-hwmonx-temp1-max-alarm)
* [/sys/bus/platform/devices/dfl-fme.0/hwmon/hwmonX/temp1\_crit\_alarm](abi-testing.html#abi-sys-bus-platform-devices-dfl-fme-0-hwmon-hwmonx-temp1-crit-alarm)
* [/sys/bus/platform/devices/dfl-fme.0/hwmon/hwmonX/temp1\_max\_policy](abi-testing.html#abi-sys-bus-platform-devices-dfl-fme-0-hwmon-hwmonx-temp1-max-policy)
* [/sys/bus/platform/devices/dfl-fme.0/hwmon/hwmonX/power1\_input](abi-testing.html#abi-sys-bus-platform-devices-dfl-fme-0-hwmon-hwmonx-power1-input)
* [/sys/bus/platform/devices/dfl-fme.0/hwmon/hwmonX/power1\_max](abi-testing.html#abi-sys-bus-platform-devices-dfl-fme-0-hwmon-hwmonx-power1-max)
* [/sys/bus/platform/devices/dfl-fme.0/hwmon/hwmonX/power1\_crit](abi-testing.html#abi-sys-bus-platform-devices-dfl-fme-0-hwmon-hwmonx-power1-crit)
* [/sys/bus/platform/devices/dfl-fme.0/hwmon/hwmonX/power1\_max\_alarm](abi-testing.html#abi-sys-bus-platform-devices-dfl-fme-0-hwmon-hwmonx-power1-max-alarm)
* [/sys/bus/platform/devices/dfl-fme.0/hwmon/hwmonX/power1\_crit\_alarm](abi-testing.html#abi-sys-bus-platform-devices-dfl-fme-0-hwmon-hwmonx-power1-crit-alarm)
* [/sys/bus/platform/devices/dfl-fme.0/hwmon/hwmonX/power1\_xeon\_limit](abi-testing.html#abi-sys-bus-platform-devices-dfl-fme-0-hwmon-hwmonx-power1-xeon-limit)
* [/sys/bus/platform/devices/dfl-fme.0/hwmon/hwmonX/power1\_fpga\_limit](abi-testing.html#abi-sys-bus-platform-devices-dfl-fme-0-hwmon-hwmonx-power1-fpga-limit)
* [/sys/bus/platform/devices/dfl-fme.0/hwmon/hwmonX/power1\_ltr](abi-testing.html#abi-sys-bus-platform-devices-dfl-fme-0-hwmon-hwmonx-power1-ltr)

## ABI file testing/sysfs-platform-dfl-port

Has the following ABI:

* [/sys/bus/platform/devices/dfl-port.0/id](abi-testing.html#abi-sys-bus-platform-devices-dfl-port-0-id)
* [/sys/bus/platform/devices/dfl-port.0/afu\_id](abi-testing.html#abi-sys-bus-platform-devices-dfl-port-0-afu-id)
* [/sys/bus/platform/devices/dfl-port.0/power\_state](abi-testing.html#abi-sys-bus-platform-devices-dfl-port-0-power-state)
* [/sys/bus/platform/devices/dfl-port.0/ap1\_event](abi-testing.html#abi-sys-bus-platform-devices-dfl-port-0-ap1-event)
* [/sys/bus/platform/devices/dfl-port.0/ap2\_event](abi-testing.html#abi-sys-bus-platform-devices-dfl-port-0-ap2-event)
* [/sys/bus/platform/devices/dfl-port.0/ltr](abi-testing.html#abi-sys-bus-platform-devices-dfl-port-0-ltr)
* [/sys/bus/platform/devices/dfl-port.0/userclk\_freqcmd](abi-testing.html#abi-sys-bus-platform-devices-dfl-port-0-userclk-freqcmd)
* [/sys/bus/platform/devices/dfl-port.0/userclk\_freqsts](abi-testing.html#abi-sys-bus-platform-devices-dfl-port-0-userclk-freqsts)
* [/sys/bus/platform/devices/dfl-port.0/userclk\_freqcntrcmd](abi-testing.html#abi-sys-bus-platform-devices-dfl-port-0-userclk-freqcntrcmd)
* [/sys/bus/platform/devices/dfl-port.0/userclk\_freqcntrsts](abi-testing.html#abi-sys-bus-platform-devices-dfl-port-0-userclk-freqcntrsts)
* [/sys/bus/platform/devices/dfl-port.0/errors/errors](abi-testing.html#abi-sys-bus-platform-devices-dfl-port-0-errors-errors)
* [/sys/bus/platform/devices/dfl-port.0/errors/first\_error](abi-testing.html#abi-sys-bus-platform-devices-dfl-port-0-errors-first-error)
* [/sys/bus/platform/devices/dfl-port.0/errors/first\_malformed\_req](abi-testing.html#abi-sys-bus-platform-devices-dfl-port-0-errors-first-malformed-req)

## ABI file testing/sysfs-platform-dptf

Has the following ABI:

* [/sys/bus/platform/devices/INT3407:00/dptf\_power/charger\_type](abi-testing.html#abi-sys-bus-platform-devices-int3407-00-dptf-power-charger-type)
* [/sys/bus/platform/devices/INT3407:00/dptf\_power/adapter\_rating\_mw](abi-testing.html#abi-sys-bus-platform-devices-int3407-00-dptf-power-adapter-rating-mw)
* [/sys/bus/platform/devices/INT3407:00/dptf\_power/max\_platform\_power\_mw](abi-testing.html#abi-sys-bus-platform-devices-int3407-00-dptf-power-max-platform-power-mw)
* [/sys/bus/platform/devices/INT3407:00/dptf\_power/platform\_power\_source](abi-testing.html#abi-sys-bus-platform-devices-int3407-00-dptf-power-platform-power-source)
* [/sys/bus/platform/devices/INT3407:00/dptf\_power/battery\_steady\_power](abi-testing.html#abi-sys-bus-platform-devices-int3407-00-dptf-power-battery-steady-power)
* [/sys/bus/platform/devices/INT3407:00/dptf\_power/rest\_of\_platform\_power\_mw](abi-testing.html#abi-sys-bus-platform-devices-int3407-00-dptf-power-rest-of-platform-power-mw)
* [/sys/bus/platform/devices/INT3407:00/dptf\_power/prochot\_confirm](abi-testing.html#abi-sys-bus-platform-devices-int3407-00-dptf-power-prochot-confirm)
* [/sys/bus/platform/devices/INT3532:00/dptf\_battery/max\_platform\_power\_mw](abi-testing.html#abi-sys-bus-platform-devices-int3532-00-dptf-battery-max-platform-power-mw)
* [/sys/bus/platform/devices/INT3532:00/dptf\_battery/max\_steady\_state\_power\_mw](abi-testing.html#abi-sys-bus-platform-devices-int3532-00-dptf-battery-max-steady-state-power-mw)
* [/sys/bus/platform/devices/INT3532:00/dptf\_battery/high\_freq\_impedance\_mohm](abi-testing.html#abi-sys-bus-platform-devices-int3532-00-dptf-battery-high-freq-impedance-mohm)
* [/sys/bus/platform/devices/INT3532:00/dptf\_battery/no\_load\_voltage\_mv](abi-testing.html#abi-sys-bus-platform-devices-int3532-00-dptf-battery-no-load-voltage-mv)
* [/sys/bus/platform/devices/INT3532:00/dptf\_battery/current\_discharge\_capbility\_ma](abi-testing.html#abi-sys-bus-platform-devices-int3532-00-dptf-battery-current-discharge-capbility-ma)
* [/sys/bus/platform/devices/INTC1045:00/pch\_fivr\_switch\_frequency/freq\_mhz\_low\_clock](abi-testing.html#abi-sys-bus-platform-devices-intc1045-00-pch-fivr-switch-frequency-freq-mhz-low-clock)
* [/sys/bus/platform/devices/INTC1045:00/pch\_fivr\_switch\_frequency/freq\_mhz\_high\_clock](abi-testing.html#abi-sys-bus-platform-devices-intc1045-00-pch-fivr-switch-frequency-freq-mhz-high-clock)
* [/sys/bus/platform/devices/INTC1045:00/pch\_fivr\_switch\_frequency/fivr\_switching\_freq\_mhz](abi-testing.html#abi-sys-bus-platform-devices-intc1045-00-pch-fivr-switch-frequency-fivr-switching-freq-mhz)
* [/sys/bus/platform/devices/INTC1045:00/pch\_fivr\_switch\_frequency/fivr\_switching\_fault\_status](abi-testing.html#abi-sys-bus-platform-devices-intc1045-00-pch-fivr-switch-frequency-fivr-switching-fault-status)
* [/sys/bus/platform/devices/INTC1045:00/pch\_fivr\_switch\_frequency/ssc\_clock\_info](abi-testing.html#abi-sys-bus-platform-devices-intc1045-00-pch-fivr-switch-frequency-ssc-clock-info)

## ABI file testing/sysfs-platform-eeepc-laptop

Has the following ABI:

* [/sys/devices/platform/eeepc/disp](abi-testing.html#abi-sys-devices-platform-eeepc-disp)
* [/sys/devices/platform/eeepc/camera](abi-testing.html#abi-sys-devices-platform-eeepc-camera)
* [/sys/devices/platform/eeepc/cardr](abi-testing.html#abi-sys-devices-platform-eeepc-cardr)
* [/sys/devices/platform/eeepc/cpufv](abi-testing.html#abi-sys-devices-platform-eeepc-cpufv)
* [/sys/devices/platform/eeepc/available\_cpufv](abi-testing.html#abi-sys-devices-platform-eeepc-available-cpufv)

## ABI file testing/sysfs-platform-hidma

Has the following ABI:

* [/sys/devices/platform/hidma-\*/chid](abi-testing.html#abi-sys-devices-platform-hidma-chid)

## ABI file testing/sysfs-platform-hidma-mgmt

Has the following ABI:

* [/sys/devices/platform/hidma-mgmt\*/chanops/chan\*/priority](abi-testing.html#abi-sys-devices-platform-hidma-mgmt-chanops-chan-priority)
* [/sys/devices/platform/hidma-mgmt\*/chanops/chan\*/weight](abi-testing.html#abi-sys-devices-platform-hidma-mgmt-chanops-chan-weight)
* [/sys/devices/platform/hidma-mgmt\*/chreset\_timeout\_cycles](abi-testing.html#abi-sys-devices-platform-hidma-mgmt-chreset-timeout-cycles)
* [/sys/devices/platform/hidma-mgmt\*/dma\_channels](abi-testing.html#abi-sys-devices-platform-hidma-mgmt-dma-channels)
* [/sys/devices/platform/hidma-mgmt\*/hw\_version\_major](abi-testing.html#abi-sys-devices-platform-hidma-mgmt-hw-version-major)
* [/sys/devices/platform/hidma-mgmt\*/hw\_version\_minor](abi-testing.html#abi-sys-devices-platform-hidma-mgmt-hw-version-minor)
* [/sys/devices/platform/hidma-mgmt\*/max\_rd\_xactions](abi-testing.html#abi-sys-devices-platform-hidma-mgmt-max-rd-xactions)
* [/sys/devices/platform/hidma-mgmt\*/max\_read\_request](abi-testing.html#abi-sys-devices-platform-hidma-mgmt-max-read-request)
* [/sys/devices/platform/hidma-mgmt\*/max\_wr\_xactions](abi-testing.html#abi-sys-devices-platform-hidma-mgmt-max-wr-xactions)
* [/sys/devices/platform/hidma-mgmt\*/max\_write\_request](abi-testing.html#abi-sys-devices-platform-hidma-mgmt-max-write-request)

## ABI file testing/sysfs-platform-i2c-demux-pinctrl

Has the following ABI:

* [/sys/devices/platform/<i2c-demux-name>/available\_masters](abi-testing.html#abi-sys-devices-platform-i2c-demux-name-available-masters)
* [/sys/devices/platform/<i2c-demux-name>/current\_master](abi-testing.html#abi-sys-devices-platform-i2c-demux-name-current-master)

## ABI file testing/sysfs-platform-ideapad-laptop

Has the following ABI:

* [/sys/bus/platform/devices/VPC2004:\*/camera\_power](abi-testing.html#abi-sys-bus-platform-devices-vpc2004-camera-power)
* [/sys/bus/platform/devices/VPC2004:\*/fan\_mode](abi-testing.html#abi-sys-bus-platform-devices-vpc2004-fan-mode)
* [/sys/bus/platform/devices/VPC2004:\*/touchpad](abi-testing.html#abi-sys-bus-platform-devices-vpc2004-touchpad)
* [/sys/bus/platform/devices/VPC2004:\*/fn\_lock](abi-testing.html#abi-sys-bus-platform-devices-vpc2004-fn-lock)
* [/sys/bus/platform/devices/VPC2004:\*/usb\_charging](abi-testing.html#abi-sys-bus-platform-devices-vpc2004-usb-charging)

## ABI file testing/sysfs-platform-intel-ifs

Device instance to test mapping
intel\_ifs\_0 -> Scan Test
intel\_ifs\_1 -> Array BIST test

Has the following ABI:

* [/sys/devices/virtual/misc/intel\_ifs\_<N>/run\_test](abi-testing.html#abi-sys-devices-virtual-misc-intel-ifs-n-run-test)
* [/sys/devices/virtual/misc/intel\_ifs\_<N>/status](abi-testing.html#abi-sys-devices-virtual-misc-intel-ifs-n-status)
* [/sys/devices/virtual/misc/intel\_ifs\_<N>/details](abi-testing.html#abi-sys-devices-virtual-misc-intel-ifs-n-details)
* [/sys/devices/virtual/misc/intel\_ifs\_<N>/image\_version](abi-testing.html#abi-sys-devices-virtual-misc-intel-ifs-n-image-version)
* [/sys/devices/virtual/misc/intel\_ifs\_<N>/current\_batch](abi-testing.html#abi-sys-devices-virtual-misc-intel-ifs-n-current-batch)

## ABI file testing/sysfs-platform-intel-pmc

Has the following ABI:

* [/sys/devices/platform/<platform>/etr3](abi-testing.html#abi-sys-devices-platform-platform-etr3)

## ABI file testing/sysfs-platform-intel-wmi-sbl-fw-update

Has the following ABI:

* [/sys/bus/wmi/devices/44FADEB1-B204-40F2-8581-394BBDC1B651[-X]/firmware\_update\_request](abi-testing.html#abi-sys-bus-wmi-devices-44fadeb1-b204-40f2-8581-394bbdc1b651-x-firmware-update-request)

## ABI file testing/sysfs-platform-intel-wmi-thunderbolt

Has the following ABI:

* [/sys/bus/wmi/devices/86CCFD48-205E-4A77-9C48-2021CBEDE341[-X]/force\_power](abi-testing.html#abi-sys-bus-wmi-devices-86ccfd48-205e-4a77-9c48-2021cbede341-x-force-power)

## ABI file testing/sysfs-platform-kim

Has the following ABI:

* [/sys/devices/platform/kim/dev\_name](abi-testing.html#abi-sys-devices-platform-kim-dev-name)
* [/sys/devices/platform/kim/baud\_rate](abi-testing.html#abi-sys-devices-platform-kim-baud-rate)
* [/sys/devices/platform/kim/flow\_cntrl](abi-testing.html#abi-sys-devices-platform-kim-flow-cntrl)
* [/sys/devices/platform/kim/install](abi-testing.html#abi-sys-devices-platform-kim-install)

## ABI file testing/sysfs-platform-lg-laptop

Has the following ABI:

* [/sys/devices/platform/lg-laptop/reader\_mode](abi-testing.html#abi-sys-devices-platform-lg-laptop-reader-mode)
* [/sys/devices/platform/lg-laptop/fn\_lock](abi-testing.html#abi-sys-devices-platform-lg-laptop-fn-lock)
* [/sys/devices/platform/lg-laptop/battery\_care\_limit](abi-testing.html#abi-sys-devices-platform-lg-laptop-battery-care-limit)
* [/sys/devices/platform/lg-laptop/fan\_mode](abi-testing.html#abi-sys-devices-platform-lg-laptop-fan-mode)
* [/sys/devices/platform/lg-laptop/usb\_charge](abi-testing.html#abi-sys-devices-platform-lg-laptop-usb-charge)

## ABI file testing/sysfs-platform-mellanox-bootctl

Has the following ABI:

* [/sys/bus/platform/devices/MLNXBF04:00/lifecycle\_state](abi-testing.html#abi-sys-bus-platform-devices-mlnxbf04-00-lifecycle-state)
* [/sys/bus/platform/devices/MLNXBF04:00/post\_reset\_wdog](abi-testing.html#abi-sys-bus-platform-devices-mlnxbf04-00-post-reset-wdog)
* [/sys/bus/platform/devices/MLNXBF04:00/reset\_action](abi-testing.html#abi-sys-bus-platform-devices-mlnxbf04-00-reset-action)
* [/sys/bus/platform/devices/MLNXBF04:00/second\_reset\_action](abi-testing.html#abi-sys-bus-platform-devices-mlnxbf04-00-second-reset-action)
* [/sys/bus/platform/devices/MLNXBF04:00/secure\_boot\_fuse\_state](abi-testing.html#abi-sys-bus-platform-devices-mlnxbf04-00-secure-boot-fuse-state)
* [/sys/bus/platform/devices/MLNXBF04:00/bootfifo](abi-testing.html#abi-sys-bus-platform-devices-mlnxbf04-00-bootfifo)
* [/sys/bus/platform/devices/MLNXBF04:00/rsh\_log](abi-testing.html#abi-sys-bus-platform-devices-mlnxbf04-00-rsh-log)
* [/sys/bus/platform/devices/MLNXBF04:00/oob\_mac](abi-testing.html#abi-sys-bus-platform-devices-mlnxbf04-00-oob-mac)
* [/sys/bus/platform/devices/MLNXBF04:00/opn](abi-testing.html#abi-sys-bus-platform-devices-mlnxbf04-00-opn)
* [/sys/bus/platform/devices/MLNXBF04:00/sku](abi-testing.html#abi-sys-bus-platform-devices-mlnxbf04-00-sku)
* [/sys/bus/platform/devices/MLNXBF04:00/modl](abi-testing.html#abi-sys-bus-platform-devices-mlnxbf04-00-modl)
* [/sys/bus/platform/devices/MLNXBF04:00/sn](abi-testing.html#abi-sys-bus-platform-devices-mlnxbf04-00-sn)
* [/sys/bus/platform/devices/MLNXBF04:00/uuid](abi-testing.html#abi-sys-bus-platform-devices-mlnxbf04-00-uuid)
* [/sys/bus/platform/devices/MLNXBF04:00/rev](abi-testing.html#abi-sys-bus-platform-devices-mlnxbf04-00-rev)
* [/sys/bus/platform/devices/MLNXBF04:00/mfg\_lock](abi-testing.html#abi-sys-bus-platform-devices-mlnxbf04-00-mfg-lock)
* [/sys/bus/platform/devices/MLNXBF04:00/rtc\_battery](abi-testing.html#abi-sys-bus-platform-devices-mlnxbf04-00-rtc-battery)

## ABI file testing/sysfs-platform-mellanox-pmc

HID Driver Description
MLNXBFD0 mlxbf-pmc Performance counters (BlueField-1)
MLNXBFD1 mlxbf-pmc Performance counters (BlueField-2)
MLNXBFD2 mlxbf-pmc Performance counters (BlueField-3)

Has the following ABI:

* [/sys/bus/platform/devices/<HID>/hwmon/hwmonX/<block>/event\_list](abi-testing.html#abi-sys-bus-platform-devices-hid-hwmon-hwmonx-block-event-list)
* [/sys/bus/platform/devices/<HID>/hwmon/hwmonX/<block>/event<N>](abi-testing.html#abi-sys-bus-platform-devices-hid-hwmon-hwmonx-block-event-n)
* [/sys/bus/platform/devices/<HID>/hwmon/hwmonX/<block>/counter<N>](abi-testing.html#abi-sys-bus-platform-devices-hid-hwmon-hwmonx-block-counter-n)
* [/sys/bus/platform/devices/<HID>/hwmon/hwmonX/<block>/enable](abi-testing.html#abi-sys-bus-platform-devices-hid-hwmon-hwmonx-block-enable)
* [/sys/bus/platform/devices/<HID>/hwmon/hwmonX/<block>/<reg>](abi-testing.html#abi-sys-bus-platform-devices-hid-hwmon-hwmonx-block-reg)
* [/sys/bus/platform/devices/<HID>/hwmon/hwmonX/<block>/count\_clock](abi-testing.html#abi-sys-bus-platform-devices-hid-hwmon-hwmonx-block-count-clock)

## ABI file testing/sysfs-platform-msi-laptop

Has the following ABI:

* [/sys/devices/platform/msi-laptop-pf/lcd\_level](abi-testing.html#abi-sys-devices-platform-msi-laptop-pf-lcd-level)
* [/sys/devices/platform/msi-laptop-pf/auto\_brightness](abi-testing.html#abi-sys-devices-platform-msi-laptop-pf-auto-brightness)
* [/sys/devices/platform/msi-laptop-pf/wlan](abi-testing.html#abi-sys-devices-platform-msi-laptop-pf-wlan)
* [/sys/devices/platform/msi-laptop-pf/bluetooth](abi-testing.html#abi-sys-devices-platform-msi-laptop-pf-bluetooth)
* [/sys/devices/platform/msi-laptop-pf/touchpad](abi-testing.html#abi-sys-devices-platform-msi-laptop-pf-touchpad)
* [/sys/devices/platform/msi-laptop-pf/turbo\_mode](abi-testing.html#abi-sys-devices-platform-msi-laptop-pf-turbo-mode)
* [/sys/devices/platform/msi-laptop-pf/eco\_mode](abi-testing.html#abi-sys-devices-platform-msi-laptop-pf-eco-mode)
* [/sys/devices/platform/msi-laptop-pf/turbo\_cooldown](abi-testing.html#abi-sys-devices-platform-msi-laptop-pf-turbo-cooldown)
* [/sys/devices/platform/msi-laptop-pf/auto\_fan](abi-testing.html#abi-sys-devices-platform-msi-laptop-pf-auto-fan)

## ABI file testing/sysfs-platform-oxp

Has the following ABI:

* [/sys/devices/platform/<platform>/tt\_toggle](abi-testing.html#abi-sys-devices-platform-platform-tt-toggle)
* [/sys/devices/platform/<platform>/tt\_led](abi-testing.html#abi-sys-devices-platform-platform-tt-led)

## ABI file testing/sysfs-platform-phy-rcar-gen3-usb2

Has the following ABI:

* [/sys/devices/platform/<phy-name>/role](abi-testing.html#abi-sys-devices-platform-phy-name-role)

## ABI file testing/sysfs-platform-power-on-reason

Has the following ABI:

* [/sys/devices/platform/.../power\_on\_reason](abi-testing.html#abi-sys-devices-platform-power-on-reason)

## ABI file testing/sysfs-platform-renesas\_usb3

Has the following ABI:

* [/sys/devices/platform/<renesas\_usb3’s name>/role](abi-testing.html#abi-sys-devices-platform-renesas-usb3-s-name-role)

## ABI file testing/sysfs-platform-silicom

Has the following ABI:

* [/sys/devices/platform/silicom-platform/uc\_version](abi-testing.html#abi-sys-devices-platform-silicom-platform-uc-version)
* [/sys/devices/platform/silicom-platform/power\_cycle](abi-testing.html#abi-sys-devices-platform-silicom-platform-power-cycle)
* [/sys/devices/platform/silicom-platform/efuse\_status](abi-testing.html#abi-sys-devices-platform-silicom-platform-efuse-status)

## ABI file testing/sysfs-platform-sst-atom

Has the following ABI:

* [/sys/devices/platform/8086<x>:00/firmware\_version](abi-testing.html#abi-sys-devices-platform-8086-x-00-firmware-version)

## ABI file testing/sysfs-platform-tahvo-usb

Has the following ABI:

* [/sys/bus/platform/devices/tahvo-usb/otg\_mode](abi-testing.html#abi-sys-bus-platform-devices-tahvo-usb-otg-mode)
* [/sys/bus/platform/devices/tahvo-usb/vbus](abi-testing.html#abi-sys-bus-platform-devices-tahvo-usb-vbus)

## ABI file testing/sysfs-platform-ts5500

Has the following ABI:

* [/sys/devices/platform/ts5500/adc](abi-testing.html#abi-sys-devices-platform-ts5500-adc)
* [/sys/devices/platform/ts5500/ereset](abi-testing.html#abi-sys-devices-platform-ts5500-ereset)
* [/sys/devices/platform/ts5500/id](abi-testing.html#abi-sys-devices-platform-ts5500-id)
* [/sys/devices/platform/ts5500/jumpers](abi-testing.html#abi-sys-devices-platform-ts5500-jumpers)
* [/sys/devices/platform/ts5500/name](abi-testing.html#abi-sys-devices-platform-ts5500-name)
* [/sys/devices/platform/ts5500/rs485](abi-testing.html#abi-sys-devices-platform-ts5500-rs485)
* [/sys/devices/platform/ts5500/sram](abi-testing.html#abi-sys-devices-platform-ts5500-sram)

## ABI file testing/sysfs-platform-twl4030-usb

Has the following ABI:

* [/sys/bus/platform/devices/\*twl4030-usb/vbus](abi-testing.html#abi-sys-bus-platform-devices-twl4030-usb-vbus)

## ABI file testing/sysfs-platform-usbip-vudc

Has the following ABI:

* [/sys/devices/platform/usbip-vudc.%d/dev\_desc](abi-testing.html#abi-sys-devices-platform-usbip-vudc-d-dev-desc)
* [/sys/devices/platform/usbip-vudc.%d/usbip\_status](abi-testing.html#abi-sys-devices-platform-usbip-vudc-d-usbip-status)
* [/sys/devices/platform/usbip-vudc.%d/usbip\_sockfd](abi-testing.html#abi-sys-devices-platform-usbip-vudc-d-usbip-sockfd)

## ABI file testing/sysfs-platform-wilco-ec

Has the following ABI:

* [/sys/bus/platform/devices/GOOG000C\:00/boot\_on\_ac](abi-testing.html#abi-sys-bus-platform-devices-goog000c-00-boot-on-ac)
* [/sys/bus/platform/devices/GOOG000C\:00/build\_date](abi-testing.html#abi-sys-bus-platform-devices-goog000c-00-build-date)
* [/sys/bus/platform/devices/GOOG000C\:00/build\_revision](abi-testing.html#abi-sys-bus-platform-devices-goog000c-00-build-revision)
* [/sys/bus/platform/devices/GOOG000C\:00/model\_number](abi-testing.html#abi-sys-bus-platform-devices-goog000c-00-model-number)
* [/sys/bus/platform/devices/GOOG000C\:00/usb\_charge](abi-testing.html#abi-sys-bus-platform-devices-goog000c-00-usb-charge)
* [/sys/bus/platform/devices/GOOG000C\:00/version](abi-testing.html#abi-sys-bus-platform-devices-goog000c-00-version)

## ABI file testing/sysfs-platform\_profile

Has the following ABI:

* [/sys/firmware/acpi/platform\_profile\_choices](abi-testing.html#abi-sys-firmware-acpi-platform-profile-choices)
* [/sys/firmware/acpi/platform\_profile](abi-testing.html#abi-sys-firmware-acpi-platform-profile)

## ABI file testing/sysfs-power

Has the following ABI:

* [/sys/power/](abi-testing.html#abi-sys-power)
* [/sys/power/state](abi-testing.html#abi-sys-power-state)
* [/sys/power/mem\_sleep](abi-testing.html#abi-sys-power-mem-sleep)
* [/sys/power/disk](abi-testing.html#abi-sys-power-disk)
* [/sys/power/image\_size](abi-testing.html#abi-sys-power-image-size)
* [/sys/power/pm\_trace](abi-testing.html#abi-sys-power-pm-trace)
* [/sys/power/pm\_trace\_dev\_match](abi-testing.html#abi-sys-power-pm-trace-dev-match)
* [/sys/power/pm\_async](abi-testing.html#abi-sys-power-pm-async)
* [/sys/power/wakeup\_count](abi-testing.html#abi-sys-power-wakeup-count)
* [/sys/power/reserved\_size](abi-testing.html#abi-sys-power-reserved-size)
* [/sys/power/autosleep](abi-testing.html#abi-sys-power-autosleep)
* [/sys/power/wake\_lock](abi-testing.html#abi-sys-power-wake-lock)
* [/sys/power/wake\_unlock](abi-testing.html#abi-sys-power-wake-unlock)
* [/sys/power/pm\_print\_times](abi-testing.html#abi-sys-power-pm-print-times)
* [/sys/power/pm\_wakeup\_irq](abi-testing.html#abi-sys-power-pm-wakeup-irq)
* [/sys/power/pm\_debug\_messages](abi-testing.html#abi-sys-power-pm-debug-messages)
* [/sys/power/resume\_offset](abi-testing.html#abi-sys-power-resume-offset)
* [/sys/power/suspend\_stats](abi-testing.html#abi-sys-power-suspend-stats)
* [/sys/power/suspend\_stats/success](abi-testing.html#abi-sys-power-suspend-stats-success)
* [/sys/power/suspend\_stats/fail](abi-testing.html#abi-sys-power-suspend-stats-fail)
* [/sys/power/suspend\_stats/failed\_freeze](abi-testing.html#abi-sys-power-suspend-stats-failed-freeze)
* [/sys/power/suspend\_stats/failed\_prepare](abi-testing.html#abi-sys-power-suspend-stats-failed-prepare)
* [/sys/power/suspend\_stats/failed\_resume](abi-testing.html#abi-sys-power-suspend-stats-failed-resume)
* [/sys/power/suspend\_stats/failed\_resume\_early](abi-testing.html#abi-sys-power-suspend-stats-failed-resume-early)
* [/sys/power/suspend\_stats/failed\_resume\_noirq](abi-testing.html#abi-sys-power-suspend-stats-failed-resume-noirq)
* [/sys/power/suspend\_stats/failed\_suspend](abi-testing.html#abi-sys-power-suspend-stats-failed-suspend)
* [/sys/power/suspend\_stats/failed\_suspend\_late](abi-testing.html#abi-sys-power-suspend-stats-failed-suspend-late)
* [/sys/power/suspend\_stats/failed\_suspend\_noirq](abi-testing.html#abi-sys-power-suspend-stats-failed-suspend-noirq)
* [/sys/power/suspend\_stats/last\_failed\_dev](abi-testing.html#abi-sys-power-suspend-stats-last-failed-dev)
* [/sys/power/suspend\_stats/last\_failed\_errno](abi-testing.html#abi-sys-power-suspend-stats-last-failed-errno)
* [/sys/power/suspend\_stats/last\_failed\_step](abi-testing.html#abi-sys-power-suspend-stats-last-failed-step)
* [/sys/power/suspend\_stats/last\_hw\_sleep](abi-testing.html#abi-sys-power-suspend-stats-last-hw-sleep)
* [/sys/power/suspend\_stats/total\_hw\_sleep](abi-testing.html#abi-sys-power-suspend-stats-total-hw-sleep)
* [/sys/power/suspend\_stats/max\_hw\_sleep](abi-testing.html#abi-sys-power-suspend-stats-max-hw-sleep)
* [/sys/power/sync\_on\_suspend](abi-testing.html#abi-sys-power-sync-on-suspend)
* [/sys/power/hibernate\_compression\_threads](abi-testing.html#abi-sys-power-hibernate-compression-threads)

## ABI file testing/sysfs-pps

Has the following ABI:

* [/sys/class/pps/](abi-testing.html#abi-sys-class-pps)
* [/sys/class/pps/ppsX/](abi-testing.html#abi-sys-class-pps-ppsx)
* [/sys/class/pps/ppsX/assert](abi-testing.html#abi-sys-class-pps-ppsx-assert)
* [/sys/class/pps/ppsX/clear](abi-testing.html#abi-sys-class-pps-ppsx-clear)
* [/sys/class/pps/ppsX/mode](abi-testing.html#abi-sys-class-pps-ppsx-mode)
* [/sys/class/pps/ppsX/echo](abi-testing.html#abi-sys-class-pps-ppsx-echo)
* [/sys/class/pps/ppsX/name](abi-testing.html#abi-sys-class-pps-ppsx-name)
* [/sys/class/pps/ppsX/path](abi-testing.html#abi-sys-class-pps-ppsx-path)

## ABI file testing/sysfs-pps-gen

Has the following ABI:

* [/sys/class/pps-gen/](abi-testing.html#abi-sys-class-pps-gen)
* [/sys/class/pps-gen/pps-genX/](abi-testing.html#abi-sys-class-pps-gen-pps-genx)
* [/sys/class/pps-gen/pps-genX/enable](abi-testing.html#abi-sys-class-pps-gen-pps-genx-enable)
* [/sys/class/pps-gen/pps-genX/system](abi-testing.html#abi-sys-class-pps-gen-pps-genx-system)
* [/sys/class/pps-gen/pps-genX/time](abi-testing.html#abi-sys-class-pps-gen-pps-genx-time)

## ABI file testing/sysfs-pps-gen-tio

Has the following ABI:

* [/sys/class/pps-gen/pps-genx/enable](abi-testing.html#abi-sys-class-pps-gen-pps-genx-enableo)

## ABI file testing/sysfs-profiling

Has the following ABI:

* [/sys/kernel/profiling](abi-testing.html#abi-sys-kernel-profiling)

## ABI file testing/sysfs-ptp

Has the following ABI:

* [/sys/class/ptp/](abi-testing.html#abi-sys-class-ptp)
* [/sys/class/ptp/ptp<N>/](abi-testing.html#abi-sys-class-ptp-ptp-n)
* [/sys/class/ptp/ptp<N>/clock\_name](abi-testing.html#abi-sys-class-ptp-ptp-n-clock-name)
* [/sys/class/ptp/ptp<N>/max\_adjustment](abi-testing.html#abi-sys-class-ptp-ptp-n-max-adjustment)
* [/sys/class/ptp/ptp<N>/max\_vclocks](abi-testing.html#abi-sys-class-ptp-ptp-n-max-vclocks)
* [/sys/class/ptp/ptp<N>/n\_alarms](abi-testing.html#abi-sys-class-ptp-ptp-n-n-alarms)
* [/sys/class/ptp/ptp<N>/n\_external\_timestamps](abi-testing.html#abi-sys-class-ptp-ptp-n-n-external-timestamps)
* [/sys/class/ptp/ptp<N>/n\_periodic\_outputs](abi-testing.html#abi-sys-class-ptp-ptp-n-n-periodic-outputs)
* [/sys/class/ptp/ptp<N>/n\_pins](abi-testing.html#abi-sys-class-ptp-ptp-n-n-pins)
* [/sys/class/ptp/ptp<N>/n\_vclocks](abi-testing.html#abi-sys-class-ptp-ptp-n-n-vclocks)
* [/sys/class/ptp/ptp<N>/pins](abi-testing.html#abi-sys-class-ptp-ptp-n-pins)
* [/sys/class/ptp/ptp<N>/pps\_available](abi-testing.html#abi-sys-class-ptp-ptp-n-pps-available)
* [/sys/class/ptp/ptp<N>/extts\_enable](abi-testing.html#abi-sys-class-ptp-ptp-n-extts-enable)
* [/sys/class/ptp/ptp<N>/fifo](abi-testing.html#abi-sys-class-ptp-ptp-n-fifo)
* [/sys/class/ptp/ptp<N>/period](abi-testing.html#abi-sys-class-ptp-ptp-n-period)
* [/sys/class/ptp/ptp<N>/pps\_enable](abi-testing.html#abi-sys-class-ptp-ptp-n-pps-enable)

## ABI file testing/sysfs-secvar

Has the following ABI:

* [/sys/firmware/secvar](abi-testing.html#abi-sys-firmware-secvar)
* [/sys/firmware/secvar/vars](abi-testing.html#abi-sys-firmware-secvar-vars)
* [/sys/firmware/secvar/format](abi-testing.html#abi-sys-firmware-secvar-format)
* [/sys/firmware/secvar/vars/<variable name>](abi-testing.html#abi-sys-firmware-secvar-vars-variable-name)
* [/sys/firmware/secvar/vars/<variable\_name>/size](abi-testing.html#abi-sys-firmware-secvar-vars-variable-name-size)
* [/sys/firmware/secvar/vars/<variable\_name>/data](abi-testing.html#abi-sys-firmware-secvar-vars-variable-name-data)
* [/sys/firmware/secvar/vars/<variable\_name>/update](abi-testing.html#abi-sys-firmware-secvar-vars-variable-name-update)

## ABI file testing/sysfs-timecard

Has the following ABI:

* [/sys/class/timecard/](abi-testing.html#abi-sys-class-timecard)
* [/sys/class/timecard/ocpN/](abi-testing.html#abi-sys-class-timecard-ocpn)
* [/sys/class/timecard/ocpN/available\_clock\_sources](abi-testing.html#abi-sys-class-timecard-ocpn-available-clock-sources)
* [/sys/class/timecard/ocpN/available\_sma\_inputs](abi-testing.html#abi-sys-class-timecard-ocpn-available-sma-inputs)
* [/sys/class/timecard/ocpN/available\_sma\_outputs](abi-testing.html#abi-sys-class-timecard-ocpn-available-sma-outputs)
* [/sys/class/timecard/ocpN/clock\_source](abi-testing.html#abi-sys-class-timecard-ocpn-clock-source)
* [/sys/class/timecard/ocpN/clock\_status\_drift](abi-testing.html#abi-sys-class-timecard-ocpn-clock-status-drift)
* [/sys/class/timecard/ocpN/clock\_status\_offset](abi-testing.html#abi-sys-class-timecard-ocpn-clock-status-offset)
* [/sys/class/timecard/ocpN/freqX](abi-testing.html#abi-sys-class-timecard-ocpn-freqx)
* [/sys/class/timecard/ocpN/freqX/frequency](abi-testing.html#abi-sys-class-timecard-ocpn-freqx-frequency)
* [/sys/class/timecard/ocpN/freqX/seconds](abi-testing.html#abi-sys-class-timecard-ocpn-freqx-seconds)
* [/sys/class/timecard/ocpN/genX](abi-testing.html#abi-sys-class-timecard-ocpn-genx)
* [/sys/class/timecard/ocpN/genX/duty](abi-testing.html#abi-sys-class-timecard-ocpn-genx-duty)
* [/sys/class/timecard/ocpN/genX/period](abi-testing.html#abi-sys-class-timecard-ocpn-genx-period)
* [/sys/class/timecard/ocpN/genX/phase](abi-testing.html#abi-sys-class-timecard-ocpn-genx-phase)
* [/sys/class/timecard/ocpN/genX/polarity](abi-testing.html#abi-sys-class-timecard-ocpn-genx-polarity)
* [/sys/class/timecard/ocpN/genX/running](abi-testing.html#abi-sys-class-timecard-ocpn-genx-running)
* [/sys/class/timecard/ocpN/genX/start](abi-testing.html#abi-sys-class-timecard-ocpn-genx-start)
* [/sys/class/timecard/ocpN/genX/signal](abi-testing.html#abi-sys-class-timecard-ocpn-genx-signal)
* [/sys/class/timecard/ocpN/gnss\_sync](abi-testing.html#abi-sys-class-timecard-ocpn-gnss-sync)
* [/sys/class/timecard/ocpN/i2c](abi-testing.html#abi-sys-class-timecard-ocpn-i2c)
* [/sys/class/timecard/ocpN/irig\_b\_mode](abi-testing.html#abi-sys-class-timecard-ocpn-irig-b-mode)
* [/sys/class/timecard/ocpN/pps](abi-testing.html#abi-sys-class-timecard-ocpn-pps)
* [/sys/class/timecard/ocpN/ptp](abi-testing.html#abi-sys-class-timecard-ocpn-ptp)
* [/sys/class/timecard/ocpN/serialnum](abi-testing.html#abi-sys-class-timecard-ocpn-serialnum)
* [/sys/class/timecard/ocpN/sma1](abi-testing.html#abi-sys-class-timecard-ocpn-sma1)
* [/sys/class/timecard/ocpN/sma2](abi-testing.html#abi-sys-class-timecard-ocpn-sma1)
* [/sys/class/timecard/ocpN/sma3](abi-testing.html#abi-sys-class-timecard-ocpn-sma1)
* [/sys/class/timecard/ocpN/sma4](abi-testing.html#abi-sys-class-timecard-ocpn-sma1)
* [/sys/class/timecard/ocpN/tod\_correction](abi-testing.html#abi-sys-class-timecard-ocpn-tod-correction)
* [/sys/class/timecard/ocpN/ts\_window\_adjust](abi-testing.html#abi-sys-class-timecard-ocpn-ts-window-adjust)
* [/sys/class/timecard/ocpN/tty](abi-testing.html#abi-sys-class-timecard-ocpn-tty)
* [/sys/class/timecard/ocpN/tty/ttyGNSS](abi-testing.html#abi-sys-class-timecard-ocpn-tty-ttygnss)
* [/sys/class/timecard/ocpN/tty/ttyGNSS2](abi-testing.html#abi-sys-class-timecard-ocpn-tty-ttygnss)
* [/sys/class/timecard/ocpN/tty/ttyMAC](abi-testing.html#abi-sys-class-timecard-ocpn-tty-ttymac)
* [/sys/class/timecard/ocpN/tty/ttyNMEA](abi-testing.html#abi-sys-class-timecard-ocpn-tty-ttynmea)
* [/sys/class/timecard/ocpN/utc\_tai\_offset](abi-testing.html#abi-sys-class-timecard-ocpn-utc-tai-offset)

## ABI file testing/sysfs-tty

Has the following ABI:

* [/sys/class/tty/console/active](abi-testing.html#abi-sys-class-tty-console-active)
* [/sys/class/tty/tty<x>/active](abi-testing.html#abi-sys-class-tty-tty-x-active)
* [/sys/class/tty/ttyS<x>/uartclk](abi-testing.html#abi-sys-class-tty-ttys-x-uartclk)
* [/sys/class/tty/ttyS<x>/type](abi-testing.html#abi-sys-class-tty-ttys-x-type)
* [/sys/class/tty/ttyS<x>/line](abi-testing.html#abi-sys-class-tty-ttys-x-line)
* [/sys/class/tty/ttyS<x>/port](abi-testing.html#abi-sys-class-tty-ttys-x-port)
* [/sys/class/tty/ttyS<x>/irq](abi-testing.html#abi-sys-class-tty-ttys-x-irq)
* [/sys/class/tty/ttyS<x>/flags](abi-testing.html#abi-sys-class-tty-ttys-x-flags)
* [/sys/class/tty/ttyS<x>/xmit\_fifo\_size](abi-testing.html#abi-sys-class-tty-ttys-x-xmit-fifo-size)
* [/sys/class/tty/ttyS<x>/close\_delay](abi-testing.html#abi-sys-class-tty-ttys-x-close-delay)
* [/sys/class/tty/ttyS<x>/closing\_wait](abi-testing.html#abi-sys-class-tty-ttys-x-closing-wait)
* [/sys/class/tty/ttyS<x>/custom\_divisor](abi-testing.html#abi-sys-class-tty-ttys-x-custom-divisor)
* [/sys/class/tty/ttyS<x>/io\_type](abi-testing.html#abi-sys-class-tty-ttys-x-io-type)
* [/sys/class/tty/ttyS<x>/iomem\_base](abi-testing.html#abi-sys-class-tty-ttys-x-iomem-base)
* [/sys/class/tty/ttyS<x>/iomem\_reg\_shift](abi-testing.html#abi-sys-class-tty-ttys-x-iomem-reg-shift)
* [/sys/class/tty/ttyS<x>/rx\_trig\_bytes](abi-testing.html#abi-sys-class-tty-ttys-x-rx-trig-bytes)
* [/sys/class/tty/ttyS<x>/console](abi-testing.html#abi-sys-class-tty-ttys-x-console)

## ABI file testing/sysfs-uevent

Has the following ABI:

* [/sys/.../uevent](abi-testing.html#abi-sys-uevent)

## ABI file testing/usb-charger-uevent

Has the following ABI:

* [Raise a uevent when a USB charger is inserted or removed](abi-testing.html#abi-raise-a-uevent-when-a-usb-charger-is-inserted-or-removed)

## ABI file testing/usb-uevent

Has the following ABI:

* [Raise a uevent when a USB Host Controller has died](abi-testing.html#abi-raise-a-uevent-when-a-usb-host-controller-has-died)
