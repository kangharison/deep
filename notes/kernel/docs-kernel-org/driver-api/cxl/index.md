# Compute Express Link

> 출처(원문): https://docs.kernel.org/driver-api/cxl/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Compute Express Link

CXL device configuration has a complex handoff between platform (Hardware,
BIOS, EFI), OS (early boot, core kernel, driver), and user policy decisions
that have impacts on each other. The docs here break up configurations steps.

Overview

* [Compute Express Link Driver Theory of Operation](theory-of-operation.html)
  + [The CXL Bus](theory-of-operation.html#the-cxl-bus)
  + [Driver Infrastructure](theory-of-operation.html#driver-infrastructure)
  + [External Interfaces](theory-of-operation.html#external-interfaces)
* [Compute Express Link Subsystem Maturity Map](maturity-map.html)
  + [Feature and Capabilities](maturity-map.html#feature-and-capabilities)
  + [Details](maturity-map.html#details)
* [Compute Express Link: Linux Conventions](conventions.html)
  + [Resolve conflict between CFMWS, Platform Memory Holes, and Endpoint Decoders](conventions/cxl-lmh.html)
  + [ACPI PRM CXL Address Translation](conventions/cxl-atl.html)
  + [Template File](conventions/template.html)

Device Reference

* [Devices and Protocols](devices/device-types.html)
  + [Protocols](devices/device-types.html#protocols)
  + [Device Types](devices/device-types.html#device-types)
  + [Example Devices](devices/device-types.html#example-devices)

Platform Configuration

* [BIOS/EFI Configuration](platform/bios-and-efi.html)
  + [Linux Expectations of BIOS/EFI Software](platform/bios-and-efi.html#linux-expectations-of-bios-efi-software)
  + [UEFI Settings](platform/bios-and-efi.html#uefi-settings)
  + [Physical Memory Map](platform/bios-and-efi.html#physical-memory-map)
  + [Decoder Programming](platform/bios-and-efi.html#decoder-programming)
* [ACPI Tables](platform/acpi.html)
  + [CEDT - CXL Early Discovery Table](platform/acpi/cedt.html)
  + [SRAT - Static Resource Affinity Table](platform/acpi/srat.html)
  + [HMAT - Heterogeneous Memory Attribute Table](platform/acpi/hmat.html)
  + [SLIT - System Locality Information Table](platform/acpi/slit.html)
  + [DSDT - Differentiated system Description Table](platform/acpi/dsdt.html)
  + [ACPI Debugging](platform/acpi.html#acpi-debugging)
* [Coherent Device Attribute Table (CDAT)](platform/cdat.html)
* [Device Scoped Memory Affinity Structure (DSMAS)](platform/cdat.html#device-scoped-memory-affinity-structure-dsmas)
* [Device Scoped Latency and Bandwidth Information Structure (DSLBIS)](platform/cdat.html#device-scoped-latency-and-bandwidth-information-structure-dslbis)
* [Switch Scoped Latency and Bandwidth Information Structure (SSLBIS)](platform/cdat.html#switch-scoped-latency-and-bandwidth-information-structure-sslbis)
* [Example Platform Configurations](platform/example-configs.html)
  + [One Device per Host Bridge](platform/example-configurations/one-dev-per-hb.html)
  + [Multiple Devices per Host Bridge](platform/example-configurations/multi-dev-per-hb.html)
  + [Cross-Host-Bridge Interleave](platform/example-configurations/hb-interleave.html)
  + [Flexible Presentation](platform/example-configurations/flexible.html)
* [CXL Device Hotplug](platform/device-hotplug.html)
  + [Hot-Remove](platform/device-hotplug.html#hot-remove)
  + [Memory Device Hot-Add](platform/device-hotplug.html#memory-device-hot-add)
  + [Interleave Sets](platform/device-hotplug.html#interleave-sets)

Linux Kernel Configuration

* [Overview](linux/overview.html)
* [Linux Init (Early Boot)](linux/early-boot.html)
  + [BIOS, Build and Boot Options](linux/early-boot.html#bios-build-and-boot-options)
  + [Memory Map Creation](linux/early-boot.html#memory-map-creation)
  + [NUMA Node Reservation](linux/early-boot.html#numa-node-reservation)
  + [Memory Tiers Creation](linux/early-boot.html#memory-tiers-creation)
  + [Contiguous Memory Allocation](linux/early-boot.html#contiguous-memory-allocation)
* [CXL Driver Operation](linux/cxl-driver.html)
  + [Drivers](linux/cxl-driver.html#drivers)
  + [Driver Devices](linux/cxl-driver.html#driver-devices)
  + [Decoder Programming](linux/cxl-driver.html#decoder-programming)
  + [Example Configurations](linux/cxl-driver.html#example-configurations)
* [DAX Driver Operation](linux/dax-driver.html)
  + [DAX Device](linux/dax-driver.html#dax-device)
  + [kmem conversion](linux/dax-driver.html#kmem-conversion)
* [Memory Hotplug](linux/memory-hotplug.html)
  + [Default Online Behavior](linux/memory-hotplug.html#default-online-behavior)
  + [Hotplug Memory Block Size](linux/memory-hotplug.html#hotplug-memory-block-size)
  + [Memory Map](linux/memory-hotplug.html#memory-map)
  + [Driver Managed Memory](linux/memory-hotplug.html#driver-managed-memory)
* [CXL Access Coordinates Computation](linux/access-coordinates.html)
  + [Latency and Bandwidth Calculation](linux/access-coordinates.html#latency-and-bandwidth-calculation)
  + [Shared Upstream Link Calculation](linux/access-coordinates.html#shared-upstream-link-calculation)
  + [QTG ID](linux/access-coordinates.html#qtg-id)

Memory Allocation

* [DAX Devices](allocation/dax.html)
* [The Page Allocator](allocation/page-allocator.html)
  + [NUMA nodes and mempolicy](allocation/page-allocator.html#numa-nodes-and-mempolicy)
  + [Memory Zones](allocation/page-allocator.html#memory-zones)
  + [CGroups and CPUSets](allocation/page-allocator.html#cgroups-and-cpusets)
* [Reclaim](allocation/reclaim.html)
  + [Demotion](allocation/reclaim.html#demotion)
  + [ZSwap and Node Preference](allocation/reclaim.html#zswap-and-node-preference)
  + [Demotion with ZSwap](allocation/reclaim.html#demotion-with-zswap)
* [Huge Pages](allocation/hugepages.html)
  + [Contiguous Memory Allocator](allocation/hugepages.html#contiguous-memory-allocator)
  + [HugeTLB](allocation/hugepages.html#hugetlb)
