# How CPU topology info is exported via sysfs

> 출처(원문): https://docs.kernel.org/admin-guide/cputopology.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# How CPU topology info is exported via sysfs

CPU topology info is exported via sysfs. Items (attributes) are similar
to /proc/cpuinfo output of some architectures. They reside in
/sys/devices/system/cpu/cpuX/topology/. Please refer to the ABI file:
[ABI file stable/sysfs-devices-system-cpu](abi-stable-files.html#abi-file-stable-sysfs-devices-system-cpu).

Architecture-neutral, drivers/base/topology.c, exports these attributes.
However the die, cluster, book, and drawer hierarchy related sysfs files will
only be created if an architecture provides the related macros as described
below.

For an architecture to support this feature, it must define some of
these macros in include/asm-XXX/topology.h:

```
#define topology_physical_package_id(cpu)
#define topology_die_id(cpu)
#define topology_cluster_id(cpu)
#define topology_core_id(cpu)
#define topology_book_id(cpu)
#define topology_drawer_id(cpu)
#define topology_sibling_cpumask(cpu)
#define topology_core_cpumask(cpu)
#define topology_cluster_cpumask(cpu)
#define topology_die_cpumask(cpu)
#define topology_book_cpumask(cpu)
#define topology_drawer_cpumask(cpu)
```

The type of `**_id macros` is int.
The type of `**_cpumask macros` is `(const) struct cpumask *`. The latter
correspond with appropriate `**_siblings` sysfs attributes (except for
`topology_sibling_cpumask()` which corresponds with thread\_siblings).

To be consistent on all architectures, include/linux/topology.h
provides default definitions for any of the above macros that are
not defined by include/asm-XXX/topology.h:

1. topology\_physical\_package\_id: -1
2. topology\_die\_id: -1
3. topology\_cluster\_id: -1
4. topology\_core\_id: 0
5. topology\_book\_id: -1
6. topology\_drawer\_id: -1
7. topology\_sibling\_cpumask: just the given CPU
8. topology\_core\_cpumask: just the given CPU
9. topology\_cluster\_cpumask: just the given CPU
10. topology\_die\_cpumask: just the given CPU
11. topology\_book\_cpumask: just the given CPU
12. topology\_drawer\_cpumask: just the given CPU

Additionally, CPU topology information is provided under
/sys/devices/system/cpu and includes these files. The internal
source for the output is in brackets (“[]”).

> |  |  |
> | --- | --- |
> | kernel\_max: | the maximum CPU index allowed by the kernel configuration. [NR\_CPUS-1] |
> | offline: | CPUs that are not online because they have been HOTPLUGGED off or exceed the limit of CPUs allowed by the kernel configuration (kernel\_max above). [~cpu\_online\_mask + cpus >= NR\_CPUS] |
> | online: | CPUs that are online and being scheduled [cpu\_online\_mask] |
> | possible: | CPUs that have been allocated resources and can be brought online if they are present. [cpu\_possible\_mask] |
> | present: | CPUs that have been identified as being present in the system. [cpu\_present\_mask] |

The format for the above output is compatible with `cpulist_parse()`
[see <linux/cpumask.h>]. Some examples follow.

In this example, there are 64 CPUs in the system but cpus 32-63 exceed
the kernel max which is limited to 0..31 by the NR\_CPUS config option
being 32. Note also that CPUs 2 and 4-31 are not online but could be
brought online as they are both present and possible:

```
kernel_max: 31
   offline: 2,4-31,32-63
    online: 0-1,3
  possible: 0-31
   present: 0-31
```

In this example, the NR\_CPUS config option is 128, but the kernel was
started with possible\_cpus=144. There are 4 CPUs in the system and cpu2
was manually taken offline (and is the only CPU that can be brought
online.):

```
kernel_max: 127
   offline: 2,4-127,128-143
    online: 0-1,3
  possible: 0-127
   present: 0-3
```

See [CPU hotplug in the Kernel](../core-api/cpu_hotplug.html) for the possible\_cpus=NUM
kernel start parameter as well as more information on the various cpumasks.
