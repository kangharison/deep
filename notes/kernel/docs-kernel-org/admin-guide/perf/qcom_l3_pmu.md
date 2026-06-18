# Qualcomm Datacenter Technologies L3 Cache Performance Monitoring Unit (PMU)

> 출처(원문): https://docs.kernel.org/admin-guide/perf/qcom_l3_pmu.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Qualcomm Datacenter Technologies L3 Cache Performance Monitoring Unit (PMU)

This driver supports the L3 cache PMUs found in Qualcomm Datacenter Technologies
Centriq SoCs. The L3 cache on these SOCs is composed of multiple slices, shared
by all cores within a socket. Each slice is exposed as a separate uncore perf
PMU with device name l3cache\_<socket>\_<instance>. User space is responsible
for aggregating across slices.

The driver provides a description of its available events and configuration
options in sysfs, see /sys/bus/event\_source/devices/l3cache\*. Given that these are uncore PMUs
the driver also exposes a “cpumask” sysfs attribute which contains a mask
consisting of one CPU per socket which will be used to handle all the PMU
events on that socket.

The hardware implements 32bit event counters and has a flat 8bit event space
exposed via the “event” format attribute. In addition to the 32bit physical
counters the driver supports virtual 64bit hardware counters by using hardware
counter chaining. This feature is exposed via the “lc” (long counter) format
flag. E.g.:

```
perf stat -e l3cache_0_0/read-miss,lc/
```

Given that these are uncore PMUs the driver does not support sampling, therefore
“perf record” will not work. Per-task perf sessions are not supported.
