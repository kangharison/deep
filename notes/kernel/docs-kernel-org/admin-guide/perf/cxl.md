# CXL Performance Monitoring Unit (CPMU)

> 출처(원문): https://docs.kernel.org/admin-guide/perf/cxl.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# CXL Performance Monitoring Unit (CPMU)

The CXL rev 3.0 specification provides a definition of CXL Performance
Monitoring Unit in section 13.2: Performance Monitoring.

CXL components (e.g. Root Port, Switch Upstream Port, End Point) may have
any number of CPMU instances. CPMU capabilities are fully discoverable from
the devices. The specification provides event definitions for all CXL protocol
message types and a set of additional events for things commonly counted on
CXL devices (e.g. DRAM events).

## CPMU driver

The CPMU driver registers a perf PMU with the name pmu\_mem<X>.<Y> on the CXL bus
representing the Yth CPMU for memX.

> /sys/bus/cxl/device/pmu\_mem<X>.<Y>

The associated PMU is registered as

> /sys/bus/event\_sources/devices/cxl\_pmu\_mem<X>.<Y>

In common with other CXL bus devices, the id has no specific meaning and the
relationship to specific CXL device should be established via the device parent
of the device on the CXL bus.

PMU driver provides description of available events and filter options in sysfs.

The “format” directory describes all formats of the config (event vendor id,
group id and mask) config1 (threshold, filter enables) and config2 (filter
parameters) fields of the perf\_event\_attr structure. The “events” directory
describes all documented events show in perf list.

The events shown in perf list are the most fine grained events with a single
bit of the event mask set. More general events may be enable by setting
multiple mask bits in config. For example, all Device to Host Read Requests
may be captured on a single counter by setting the bits for all of

* d2h\_req\_rdcurr
* d2h\_req\_rdown
* d2h\_req\_rdshared
* d2h\_req\_rdany
* d2h\_req\_rdownnodata

Example of usage:

```
$#perf list
cxl_pmu_mem0.0/clock_ticks/                        [Kernel PMU event]
cxl_pmu_mem0.0/d2h_req_rdshared/                   [Kernel PMU event]
cxl_pmu_mem0.0/h2d_req_snpcur/                     [Kernel PMU event]
cxl_pmu_mem0.0/h2d_req_snpdata/                    [Kernel PMU event]
cxl_pmu_mem0.0/h2d_req_snpinv/                     [Kernel PMU event]
-----------------------------------------------------------

$# perf stat -a -e cxl_pmu_mem0.0/clock_ticks/ -e cxl_pmu_mem0.0/d2h_req_rdshared/
```

Vendor specific events may also be available and if so can be used via

> $# perf stat -a -e cxl\_pmu\_mem0.0/vid=VID,gid=GID,mask=MASK/

The driver does not support sampling so “perf record” is unsupported.
It only supports system-wide counting so attaching to a task is
unsupported.
