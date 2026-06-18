# 如何通過sysfs將CPU拓撲導出

> 출처(원문): https://docs.kernel.org/translations/zh_TW/admin-guide/cputopology.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Warning

此文件的目的是爲讓中文讀者更容易閱讀和理解，而不是作爲一個分支。因此，
如果您對此文件有任何意見或改動，請先嘗試更新原始英文文件。如果要更改或
修正某處翻譯文件，請將意見或補丁發送給維護者（聯繫方式見下）。

Note

如果您發現本文檔與原始文件有任何不同或者有翻譯問題，請聯繫該文件的譯者，
或者發送電子郵件給胡皓文以獲取幫助：<[2023002089@link.tyut.edu.cn](mailto:2023002089%40link.tyut.edu.cn)>。

Original:
:   [How CPU topology info is exported via sysfs](../../../admin-guide/cputopology.html)

翻譯:
:   唐藝舟 Tang Yizhou <[tangyeechou@gmail.com](mailto:tangyeechou%40gmail.com)>

# 如何通過sysfs將CPU拓撲導出

CPU拓撲信息通過sysfs導出。顯示的項（屬性）和某些架構的/proc/cpuinfo輸出相似。它們位於
/sys/devices/system/cpu/cpuX/topology/。請閱讀ABI文件：
[ABI file stable/sysfs-devices-system-cpu](../../../admin-guide/abi-stable-files.html#abi-file-stable-sysfs-devices-system-cpu)。

drivers/base/topology.c是體系結構中性的，它導出了這些屬性。然而，die、cluster、book、
draw這些層次結構相關的文件僅在體系結構提供了下文描述的宏的條件下被創建。

對於支持這個特性的體系結構，它必須在include/asm-XXX/topology.h中定義這些宏中的一部分:

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

`**_id macros` 的類型是int。
`**_cpumask macros` 的類型是 `(const) struct cpumask *` 。後者和恰當的
`**_siblings` sysfs屬性對應（除了`topology_sibling_cpumask()`，它和thread\_siblings
對應）。

爲了在所有體系結構上保持一致，include/linux/topology.h提供了上述所有宏的默認定義，以防
它們未在include/asm-XXX/topology.h中定義:

1. topology\_physical\_package\_id: -1
2. topology\_die\_id: -1
3. topology\_cluster\_id: -1
4. topology\_core\_id: 0
5. topology\_book\_id: -1
6. topology\_drawer\_id: -1
7. topology\_sibling\_cpumask: 僅入參CPU
8. topology\_core\_cpumask: 僅入參CPU
9. topology\_cluster\_cpumask: 僅入參CPU
10. topology\_die\_cpumask: 僅入參CPU
11. topology\_book\_cpumask: 僅入參CPU
12. topology\_drawer\_cpumask: 僅入參CPU

此外，CPU拓撲信息由/sys/devices/system/cpu提供，包含下述文件。輸出對應的內部數據源放在
方括號（”[]”）中。

> |  |  |
> | --- | --- |
> | kernel\_max: | 內核配置允許的最大CPU下標值。[NR\_CPUS-1] |
> | offline: | 由於熱插拔移除或者超過內核允許的CPU上限（上文描述的kernel\_max） 導致未上線的CPU。[~cpu\_online\_mask + cpus >= NR\_CPUS] |
> | online: | 在線的CPU，可供調度使用。[cpu\_online\_mask] |
> | possible: | 已被分配資源的CPU，如果它們CPU實際存在，可以上線。 [cpu\_possible\_mask] |
> | present: | 被系統識別實際存在的CPU。[cpu\_present\_mask] |

上述輸出的格式和`cpulist_parse()`兼容[參見 <linux/cpumask.h>]。下面給些例子。

在本例中，系統中有64個CPU，但是CPU 32-63超過了kernel\_max值，因爲NR\_CPUS配置項是32，
取值範圍被限制爲0..31。此外注意CPU2和4-31未上線，但是可以上線，因爲它們同時存在於
present和possible:

```
kernel_max: 31
   offline: 2,4-31,32-63
    online: 0-1,3
  possible: 0-31
   present: 0-31
```

在本例中，NR\_CPUS配置項是128，但內核啓動時設置possible\_cpus=144。系統中有4個CPU，
CPU2被手動設置下線（也是唯一一個可以上線的CPU）:

```
kernel_max: 127
   offline: 2,4-127,128-143
    online: 0-1,3
  possible: 0-127
   present: 0-3
```

閱讀Documentation/core-api/cpu\_hotplug.rst可瞭解開機參數possible\_cpus=NUM，同時還
可以瞭解各種cpumask的信息。
