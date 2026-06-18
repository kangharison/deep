# 21.Hardware Feedback Interface For Hetero Core Scheduling On AMD Platform

> 출처(원문): https://docs.kernel.org/arch/x86/amd-hfi.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 21. Hardware Feedback Interface For Hetero Core Scheduling On AMD Platform

Copyright:
:   2025 Advanced Micro Devices, Inc. All Rights Reserved.

Author:
:   Perry Yuan <[perry.yuan@amd.com](mailto:perry.yuan%40amd.com)>

Author:
:   Mario Limonciello <[mario.limonciello@amd.com](mailto:mario.limonciello%40amd.com)>

## 21.1. Overview

AMD Heterogeneous Core implementations are comprised of more than one
architectural class and CPUs are comprised of cores of various efficiency and
power capabilities: performance-oriented *classic cores* and power-efficient
*dense cores*. As such, power management strategies must be designed to
accommodate the complexities introduced by incorporating different core types.
Heterogeneous systems can also extend to more than two architectural classes
as well. The purpose of the scheduling feedback mechanism is to provide
information to the operating system scheduler in real time such that the
scheduler can direct threads to the optimal core.

The goal of AMD’s heterogeneous architecture is to attain power benefit by
sending background threads to the dense cores while sending high priority
threads to the classic cores. From a performance perspective, sending
background threads to dense cores can free up power headroom and allow the
classic cores to optimally service demanding threads. Furthermore, the area
optimized nature of the dense cores allows for an increasing number of
physical cores. This improved core density will have positive multithreaded
performance impact.

## 21.2. AMD Heterogeneous Core Driver

The `amd_hfi` driver delivers the operating system a performance and energy
efficiency capability data for each CPU in the system. The scheduler can use
the ranking data from the HFI driver to make task placement decisions.

## 21.3. Thread Classification and Ranking Table Interaction

The thread classification is used to select into a ranking table that
describes an efficiency and performance ranking for each classification.

Threads are classified during runtime into enumerated classes. The classes
represent thread performance/power characteristics that may benefit from
special scheduling behaviors. The below table depicts an example of thread
classification and a preference where a given thread should be scheduled
based on its thread class. The real time thread classification is consumed
by the operating system and is used to inform the scheduler of where the
thread should be placed.

### 21.3.1. Thread Classification Example Table

|  |  |  |  |  |
| --- | --- | --- | --- | --- |
| class ID | Classification | Preferred scheduling behavior | Preemption priority | Counter |
| 0 | Default | Performant | Highest |  |
| 1 | Non-scalable | Efficient | Lowest | PMCx1A1 |
| 2 | I/O bound | Efficient | Lowest | PMCx044 |

Thread classification is performed by the hardware each time that the thread is switched out.
Threads that don’t meet any hardware specified criteria are classified as “default”.

## 21.4. AMD Hardware Feedback Interface

The Hardware Feedback Interface provides to the operating system information
about the performance and energy efficiency of each CPU in the system. Each
capability is given as a unit-less quantity in the range [0-255]. A higher
performance value indicates higher performance capability, and a higher
efficiency value indicates more efficiency. Energy efficiency and performance
are reported in separate capabilities in the shared memory based ranking table.

These capabilities may change at runtime as a result of changes in the
operating conditions of the system or the action of external factors.
Power Management firmware is responsible for detecting events that require
a reordering of the performance and efficiency ranking. Table updates happen
relatively infrequently and occur on the time scale of seconds or more.

The following events trigger a table update:
:   * Thermal Stress Events
    * Silent Compute
    * Extreme Low Battery Scenarios

The kernel or a userspace policy daemon can use these capabilities to modify
task placement decisions. For instance, if either the performance or energy
capabilities of a given logical processor becomes zero, it is an indication
that the hardware recommends to the operating system to not schedule any tasks
on that processor for performance or energy efficiency reasons, respectively.

## 21.5. Implementation details for Linux

The implementation of threads scheduling consists of the following steps:

1. A thread is spawned and scheduled to the ideal core using the default
   heterogeneous scheduling policy.
2. The processor profiles thread execution and assigns an enumerated
   classification ID.
   This classification is communicated to the OS via logical processor
   scope MSR.
3. During the thread context switch out the operating system consumes the
   workload (WL) classification which resides in a logical processor scope MSR.
4. The OS triggers the hardware to clear its history by writing to an MSR,
   after consuming the WL classification and before switching in the new thread.
5. If due to the classification, ranking table, and processor availability,
   the thread is not on its ideal processor, the OS will then consider
   scheduling the thread on its ideal processor (if available).

## 21.6. Ranking Table

The ranking table is a shared memory region that is used to communicate the
performance and energy efficiency capabilities of each CPU in the system.

The ranking table design includes rankings for each APIC ID in the system and
rankings both for performance and efficiency for each workload classification.

struct amd\_shmem\_info
:   Shared memory table for AMD HFI

**Definition**:

```
struct amd_shmem_info {
    struct acpi_pcct_ext_pcc_shared_memory header;
    u32 version_number          :8, n_logical_processors    :8, n_capabilities          :8, table_update_context    :8;
    u32 n_bitmaps               :8, reserved                :24;
    u32 table_data[];
};
```

**Members**

`header`
:   The PCCT table header including signature, length flags and command.

`version_number`
:   Version number of the table

`n_logical_processors`
:   Number of logical processors

`n_capabilities`
:   Number of ranking dimensions (performance, efficiency, etc)

`table_update_context`
:   Command being sent over the subspace

`n_bitmaps`
:   Number of 32-bit bitmaps to enumerate all the APIC IDs
    This is based on the maximum APIC ID enumerated in the system

`reserved`
:   24 bit spare

`table_data`
:   Bit Map(s) of enabled logical processors
    Followed by the ranking data for each logical processor

## 21.7. Ranking Table update

The power management firmware issues an platform interrupt after updating the
ranking table and is ready for the operating system to consume it. CPUs receive
such interrupt and read new ranking table from shared memory which PCCT table
has provided, then `amd_hfi` driver parses the new table to provide new
consume data for scheduling decisions.
