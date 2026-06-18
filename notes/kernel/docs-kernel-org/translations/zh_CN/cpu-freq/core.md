# CPUFreq核心和CPUFreq通知器的通用说明

> 출처(원문): https://docs.kernel.org/translations/zh_CN/cpu-freq/core.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Note

此文件的目的是为让中文读者更容易阅读和理解，而不是作为一个分支。 因此，
如果您对此文件有任何意见或更新，请先尝试更新原始英文文件。
如果您发现本文档与原始文件有任何不同或者有翻译问题，请发建议或者补丁给
该文件的译者，或者请求中文文档维护者和审阅者的帮助。

Original:
:   [General description of the CPUFreq core and CPUFreq notifiers](../../../cpu-freq/core.html)

翻译:
:   司延腾 Yanteng Si <[siyanteng@loongson.cn](mailto:siyanteng%40loongson.cn)>

校译:
:   唐艺舟 Tang Yizhou <[tangyeechou@gmail.com](mailto:tangyeechou%40gmail.com)>

# CPUFreq核心和CPUFreq通知器的通用说明

作者:
:   * Dominik Brodowski <[linux@brodo.de](mailto:linux%40brodo.de)>
    * David Kimdon <[dwhedon@debian.org](mailto:dwhedon%40debian.org)>
    * Rafael J. Wysocki <[rafael.j.wysocki@intel.com](mailto:rafael.j.wysocki%40intel.com)>
    * Viresh Kumar <[viresh.kumar@linaro.org](mailto:viresh.kumar%40linaro.org)>

## 1. CPUFreq核心和接口

cpufreq核心代码位于drivers/cpufreq/cpufreq.c中。这些cpufreq代码为CPUFreq架构的驱
动程序（那些执行硬件频率切换的代码）以及 “通知器” 提供了一个标准化的接口。
包括设备驱动程序；需要了解策略变化（如 ACPI 热量管理），或所有频率变化（如计时代码），
甚至需要强制限制为指定频率（如 ARM 架构上的 LCD 驱动程序）的其它内核组件。
此外，内核 “常数” loops\_per\_jiffy 会根据频率变化而更新。

cpufreq策略的引用计数由 cpufreq\_cpu\_get 和 cpufreq\_cpu\_put 来完成，以确保 cpufreq 驱
动程序被正确地注册到核心中，并且驱动程序在 cpufreq\_put\_cpu 被调用之前不会被卸载。这也保证
了每个CPU核的cpufreq 策略在使用期间不会被释放。

## 2. CPUFreq 通知器

CPUFreq通知器遵循标准的内核通知器接口。
关于通知器的细节请参阅 linux/include/linux/notifier.h。

这里有两个不同的CPUfreq通知器 - 策略通知器和转换通知器。

### 2.1 CPUFreq策略通知器

当创建或移除策略时，这些都会被通知。

阶段是在通知器的第二个参数中指定的。当第一次创建策略时，阶段是CPUFREQ\_CREATE\_POLICY，当
策略被移除时，阶段是CPUFREQ\_REMOVE\_POLICY。

第三个参数 `void *pointer` 指向一个结构体cpufreq\_policy，其包括min，max(新策略的下限和
上限（单位为kHz）)这几个值。

### 2.2 CPUFreq转换通知器

当CPUfreq驱动切换CPU核心频率时，策略中的每个在线CPU都会收到两次通知，这些变化没有任何外部干
预。

第二个参数指定阶段 - CPUFREQ\_PRECHANGE or CPUFREQ\_POSTCHANGE.

第三个参数是一个包含如下值的结构体cpufreq\_freqs：

|  |  |
| --- | --- |
| policy | 指向`struct cpufreq_policy`的指针 |
| old | 旧频率 |
| new | 新频率 |
| flags | cpufreq驱动的标志 |

## 3. 含有Operating Performance Point (OPP)的CPUFreq表的生成

关于OPP的细节请参阅 [Operating Performance Points (OPP) Library](../../../power/opp.html)

dev\_pm\_opp\_init\_cpufreq\_table -
:   这个函数提供了一个随时可用的转换例程，用来将OPP层关于可用频率的内部信息翻译成一种
    cpufreq易于处理的格式。

    Warning

    不要在中断上下文中使用此函数。

    例如:

    ```
    soc_pm_init()
    {
           /* Do things */
           r = dev_pm_opp_init_cpufreq_table(dev, &freq_table);
           if (!r)
                   policy->freq_table = freq_table;
           /* Do other things */
    }
    ```

    Note

    该函数只有在CONFIG\_PM\_OPP之外还启用了CONFIG\_CPU\_FREQ时才可用。

dev\_pm\_opp\_free\_cpufreq\_table
:   释放dev\_pm\_opp\_init\_cpufreq\_table分配的表。
