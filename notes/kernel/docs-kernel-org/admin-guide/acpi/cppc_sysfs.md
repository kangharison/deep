# Collaborative Processor Performance Control (CPPC)

> 출처(원문): https://docs.kernel.org/admin-guide/acpi/cppc_sysfs.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Collaborative Processor Performance Control (CPPC)

## CPPC

CPPC defined in the ACPI spec describes a mechanism for the OS to manage the
performance of a logical processor on a contiguous and abstract performance
scale. CPPC exposes a set of registers to describe abstract performance scale,
to request performance levels and to measure per-cpu delivered performance.

For more details on CPPC please refer to the ACPI specification at:

<http://uefi.org/specifications>

Some of the CPPC registers are exposed via sysfs under:

```
/sys/devices/system/cpu/cpuX/acpi_cppc/
```

for each cpu X:

```
$ ls -lR  /sys/devices/system/cpu/cpu0/acpi_cppc/
/sys/devices/system/cpu/cpu0/acpi_cppc/:
total 0
-r--r--r-- 1 root root 65536 Mar  5 19:38 feedback_ctrs
-r--r--r-- 1 root root 65536 Mar  5 19:38 highest_perf
-r--r--r-- 1 root root 65536 Mar  5 19:38 lowest_freq
-r--r--r-- 1 root root 65536 Mar  5 19:38 lowest_nonlinear_perf
-r--r--r-- 1 root root 65536 Mar  5 19:38 lowest_perf
-r--r--r-- 1 root root 65536 Mar  5 19:38 nominal_freq
-r--r--r-- 1 root root 65536 Mar  5 19:38 nominal_perf
-r--r--r-- 1 root root 65536 Mar  5 19:38 reference_perf
-r--r--r-- 1 root root 65536 Mar  5 19:38 wraparound_time
```

* highest\_perf : Highest performance of this processor (abstract scale).
* nominal\_perf : Highest sustained performance of this processor
  (abstract scale).
* lowest\_nonlinear\_perf : Lowest performance of this processor with nonlinear
  power savings (abstract scale).
* lowest\_perf : Lowest performance of this processor (abstract scale).
* lowest\_freq : CPU frequency corresponding to lowest\_perf (in MHz).
* nominal\_freq : CPU frequency corresponding to nominal\_perf (in MHz).
  The above frequencies should only be used to report processor performance in
  frequency instead of abstract scale. These values should not be used for any
  functional decisions.
* feedback\_ctrs : Includes both Reference and delivered performance counter.
  Reference counter ticks up proportional to processor’s reference performance.
  Delivered counter ticks up proportional to processor’s delivered performance.
* wraparound\_time: Minimum time for the feedback counters to wraparound
  (seconds).
* reference\_perf : Performance level at which reference performance counter
  accumulates (abstract scale).

## Computing Average Delivered Performance

Below describes the steps to compute the average performance delivered by
taking two different snapshots of feedback counters at time T1 and T2.

> T1: Read feedback\_ctrs as fbc\_t1
> :   Wait or run some workload
>
> T2: Read feedback\_ctrs as fbc\_t2

```
delivered_counter_delta = fbc_t2[del] - fbc_t1[del]
reference_counter_delta = fbc_t2[ref] - fbc_t1[ref]

delivered_perf = (reference_perf x delivered_counter_delta) / reference_counter_delta
```
