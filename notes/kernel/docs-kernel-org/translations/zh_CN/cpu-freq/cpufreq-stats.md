# sysfs CPUFreq Stats的一般说明

> 출처(원문): https://docs.kernel.org/translations/zh_CN/cpu-freq/cpufreq-stats.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Note

此文件的目的是为让中文读者更容易阅读和理解，而不是作为一个分支。 因此，
如果您对此文件有任何意见或更新，请先尝试更新原始英文文件。
如果您发现本文档与原始文件有任何不同或者有翻译问题，请发建议或者补丁给
该文件的译者，或者请求中文文档维护者和审阅者的帮助。

Original:
:   [General Description of sysfs CPUFreq Stats](../../../cpu-freq/cpufreq-stats.html)

翻译:
:   司延腾 Yanteng Si <[siyanteng@loongson.cn](mailto:siyanteng%40loongson.cn)>

校译:
:   唐艺舟 Tang Yizhou <[tangyeechou@gmail.com](mailto:tangyeechou%40gmail.com)>

# sysfs CPUFreq Stats的一般说明

为使用者准备的信息

作者: Venkatesh Pallipadi <[venkatesh.pallipadi@intel.com](mailto:venkatesh.pallipadi%40intel.com)>

## 1. 简介

cpufreq-stats是一种为每个CPU提供CPU频率统计的驱动。
这些统计数据以/sysfs中一系列只读接口的形式呈现。cpufreq-stats接口（若已配置）将为每个CPU生成
/sysfs（<sysfs root>/devices/system/cpu/cpuX/cpufreq/stats/）中cpufreq目录下的stats目录。
各项统计数据将在stats目录下形成对应的只读文件。

此驱动是以独立于任何可能运行在你所用CPU上的特定cpufreq\_driver的方式设计的。因此，它将能和任何
cpufreq\_driver协同工作。

## 2. 已提供的统计数据(有例子)

cpufreq stats提供了以下统计数据（在下面详细解释）。

* time\_in\_state
* total\_trans
* trans\_table

所有统计数据来自以下时间范围：从统计驱动被加载的时间（或统计数据被重置的时间）开始，到某一统计数据被读取的时间为止。
显然，统计驱动不会保存它被加载之前的任何频率转换信息。

```
<mysystem>:/sys/devices/system/cpu/cpu0/cpufreq/stats # ls -l
total 0
drwxr-xr-x  2 root root    0 May 14 16:06 .
drwxr-xr-x  3 root root    0 May 14 15:58 ..
--w-------  1 root root 4096 May 14 16:06 reset
-r--r--r--  1 root root 4096 May 14 16:06 time_in_state
-r--r--r--  1 root root 4096 May 14 16:06 total_trans
-r--r--r--  1 root root 4096 May 14 16:06 trans_table
```

* **reset**

只写属性，可用于重置统计计数器。这对于评估不同调节器的系统行为非常有用，且无需重启。

* **time\_in\_state**

此文件给出了在本CPU支持的每个频率上分别花费的时间。cat输出的每一行都是一个”<frequency>
<time>”对，表示这个CPU在<frequency>上花费了<time>个usertime单位的时间。输出的每一行对应
一个CPU支持的频率。这里usertime单位是10mS（类似于/proc导出的其它时间）。

```
<mysystem>:/sys/devices/system/cpu/cpu0/cpufreq/stats # cat time_in_state
3600000 2089
3400000 136
3200000 34
3000000 67
2800000 172488
```

* **total\_trans**

此文件给出了这个CPU频率转换的总次数。cat的输出是一个计数值，它就是频率转换的总次数。

```
<mysystem>:/sys/devices/system/cpu/cpu0/cpufreq/stats # cat total_trans
20
```

* **trans\_table**

本文件提供所有CPU频率转换的细粒度信息。这里的cat输出是一个二维矩阵，其中一个条目<i, j>（第
i行，第j列）代表从Freq\_i到Freq\_j的转换次数。Freq\_i行和Freq\_j列遵循驱动最初提供给cpufreq
核心的频率表的排列顺序，因此可以已排序（升序或降序）或未排序。这里的输出也包含了实际
频率值，分别按行和按列显示，以便更好地阅读。

如果转换表大于PAGE\_SIZE，读取时将返回一个-EFBIG错误。

```
<mysystem>:/sys/devices/system/cpu/cpu0/cpufreq/stats # cat trans_table
From  :    To
        :   3600000   3400000   3200000   3000000   2800000
3600000:         0         5         0         0         0
3400000:         4         0         2         0         0
3200000:         0         1         0         2         0
3000000:         0         0         1         0         3
2800000:         0         0         0         2         0
```

## 3. 配置cpufreq-stats

按以下方式在你的内核中配置cpufreq-stats:

```
Config Main Menu
        Power management options (ACPI, APM)  --->
                CPU Frequency scaling  --->
                        [*] CPU Frequency scaling
                        [*]   CPU frequency translation statistics
```

“CPU Frequency scaling” (CONFIG\_CPU\_FREQ) 应该被启用，以支持配置cpufreq-stats。

“CPU frequency translation statistics” (CONFIG\_CPU\_FREQ\_STAT)提供了包括
time\_in\_state、total\_trans和trans\_table的统计数据。

一旦启用了这个选项，并且你的CPU支持cpufrequency，你就可以在/sysfs中看到CPU频率统计。
