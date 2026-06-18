# 如何通过sysfs将CPU拓扑导出

> 출처(원문): https://docs.kernel.org/translations/zh_CN/admin-guide/cputopology.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Note

此文件的目的是为让中文读者更容易阅读和理解，而不是作为一个分支。 因此，
如果您对此文件有任何意见或更新，请先尝试更新原始英文文件。
如果您发现本文档与原始文件有任何不同或者有翻译问题，请发建议或者补丁给
该文件的译者，或者请求中文文档维护者和审阅者的帮助。

Original:
:   [How CPU topology info is exported via sysfs](../../../admin-guide/cputopology.html)

翻译:
:   唐艺舟 Tang Yizhou <[tangyeechou@gmail.com](mailto:tangyeechou%40gmail.com)>

# 如何通过sysfs将CPU拓扑导出

CPU拓扑信息通过sysfs导出。显示的项（属性）和某些架构的/proc/cpuinfo输出相似。它们位于
/sys/devices/system/cpu/cpuX/topology/。请阅读ABI文件：
[ABI file stable/sysfs-devices-system-cpu](../../../admin-guide/abi-stable-files.html#abi-file-stable-sysfs-devices-system-cpu)。

drivers/base/topology.c是体系结构中性的，它导出了这些属性。然而，die、cluster、book、
draw这些层次结构相关的文件仅在体系结构提供了下文描述的宏的条件下被创建。

对于支持这个特性的体系结构，它必须在include/asm-XXX/topology.h中定义这些宏中的一部分:

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

`**_id macros` 的类型是int。
`**_cpumask macros` 的类型是 `(const) struct cpumask *` 。后者和恰当的
`**_siblings` sysfs属性对应（除了`topology_sibling_cpumask()`，它和thread\_siblings
对应）。

为了在所有体系结构上保持一致，include/linux/topology.h提供了上述所有宏的默认定义，以防
它们未在include/asm-XXX/topology.h中定义:

1. topology\_physical\_package\_id: -1
2. topology\_die\_id: -1
3. topology\_cluster\_id: -1
4. topology\_core\_id: 0
5. topology\_book\_id: -1
6. topology\_drawer\_id: -1
7. topology\_sibling\_cpumask: 仅入参CPU
8. topology\_core\_cpumask: 仅入参CPU
9. topology\_cluster\_cpumask: 仅入参CPU
10. topology\_die\_cpumask: 仅入参CPU
11. topology\_book\_cpumask: 仅入参CPU
12. topology\_drawer\_cpumask: 仅入参CPU

此外，CPU拓扑信息由/sys/devices/system/cpu提供，包含下述文件。输出对应的内部数据源放在
方括号（”[]”）中。

> |  |  |
> | --- | --- |
> | kernel\_max: | 内核配置允许的最大CPU下标值。[NR\_CPUS-1] |
> | offline: | 由于热插拔移除或者超过内核允许的CPU上限（上文描述的kernel\_max） 导致未上线的CPU。[~cpu\_online\_mask + cpus >= NR\_CPUS] |
> | online: | 在线的CPU，可供调度使用。[cpu\_online\_mask] |
> | possible: | 已被分配资源的CPU，如果它们CPU实际存在，可以上线。 [cpu\_possible\_mask] |
> | present: | 被系统识别实际存在的CPU。[cpu\_present\_mask] |

上述输出的格式和`cpulist_parse()`兼容[参见 <linux/cpumask.h>]。下面给些例子。

在本例中，系统中有64个CPU，但是CPU 32-63超过了kernel\_max值，因为NR\_CPUS配置项是32，
取值范围被限制为0..31。此外注意CPU2和4-31未上线，但是可以上线，因为它们同时存在于
present和possible:

```
kernel_max: 31
   offline: 2,4-31,32-63
    online: 0-1,3
  possible: 0-31
   present: 0-31
```

在本例中，NR\_CPUS配置项是128，但内核启动时设置possible\_cpus=144。系统中有4个CPU，
CPU2被手动设置下线（也是唯一一个可以上线的CPU）:

```
kernel_max: 127
   offline: 2,4-127,128-143
    online: 0-1,3
  possible: 0-127
   present: 0-3
```

阅读Documentation/core-api/cpu\_hotplug.rst可了解开机参数possible\_cpus=NUM，同时还
可以了解各种cpumask的信息。
