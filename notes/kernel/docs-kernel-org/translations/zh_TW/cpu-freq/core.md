# CPUFreq核心和CPUFreq通知器的通用說明

> 출처(원문): https://docs.kernel.org/translations/zh_TW/cpu-freq/core.html
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
:   [General description of the CPUFreq core and CPUFreq notifiers](../../../cpu-freq/core.html)

翻譯:
:   司延騰 Yanteng Si <[siyanteng@loongson.cn](mailto:siyanteng%40loongson.cn)>

校譯:
:   唐藝舟 Tang Yizhou <[tangyeechou@gmail.com](mailto:tangyeechou%40gmail.com)>

# CPUFreq核心和CPUFreq通知器的通用說明

作者:
:   * Dominik Brodowski <[linux@brodo.de](mailto:linux%40brodo.de)>
    * David Kimdon <[dwhedon@debian.org](mailto:dwhedon%40debian.org)>
    * Rafael J. Wysocki <[rafael.j.wysocki@intel.com](mailto:rafael.j.wysocki%40intel.com)>
    * Viresh Kumar <[viresh.kumar@linaro.org](mailto:viresh.kumar%40linaro.org)>

## 1. CPUFreq核心和接口

cpufreq核心代碼位於drivers/cpufreq/cpufreq.c中。這些cpufreq代碼爲CPUFreq架構的驅
動程序（那些執行硬件頻率切換的代碼）以及 “通知器” 提供了一個標準化的接口。
包括設備驅動程序；需要了解策略變化（如 ACPI 熱量管理），或所有頻率變化（如計時代碼），
甚至需要強制限制爲指定頻率（如 ARM 架構上的 LCD 驅動程序）的其它內核組件。
此外，內核 “常數” loops\_per\_jiffy 會根據頻率變化而更新。

cpufreq策略的引用計數由 cpufreq\_cpu\_get 和 cpufreq\_cpu\_put 來完成，以確保 cpufreq 驅
動程序被正確地註冊到核心中，並且驅動程序在 cpufreq\_put\_cpu 被調用之前不會被卸載。這也保證
了每個CPU核的cpufreq 策略在使用期間不會被釋放。

## 2. CPUFreq 通知器

CPUFreq通知器遵循標準的內核通知器接口。
關於通知器的細節請參閱 linux/include/linux/notifier.h。

這裏有兩個不同的CPUfreq通知器 - 策略通知器和轉換通知器。

### 2.1 CPUFreq策略通知器

當創建或移除策略時，這些都會被通知。

階段是在通知器的第二個參數中指定的。當第一次創建策略時，階段是CPUFREQ\_CREATE\_POLICY，當
策略被移除時，階段是CPUFREQ\_REMOVE\_POLICY。

第三個參數 `void *pointer` 指向一個結構體cpufreq\_policy，其包括min，max(新策略的下限和
上限（單位爲kHz）)這幾個值。

### 2.2 CPUFreq轉換通知器

當CPUfreq驅動切換CPU核心頻率時，策略中的每個在線CPU都會收到兩次通知，這些變化沒有任何外部幹
預。

第二個參數指定階段 - CPUFREQ\_PRECHANGE or CPUFREQ\_POSTCHANGE.

第三個參數是一個包含如下值的結構體cpufreq\_freqs：

|  |  |
| --- | --- |
| policy | 指向`struct cpufreq_policy`的指針 |
| old | 舊頻率 |
| new | 新頻率 |
| flags | cpufreq驅動的標誌 |

## 3. 含有Operating Performance Point (OPP)的CPUFreq表的生成

關於OPP的細節請參閱 [Operating Performance Points (OPP) Library](../../../power/opp.html)

dev\_pm\_opp\_init\_cpufreq\_table -
:   這個函數提供了一個隨時可用的轉換例程，用來將OPP層關於可用頻率的內部信息翻譯成一種
    cpufreq易於處理的格式。

    Warning

    不要在中斷上下文中使用此函數。

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

    該函數只有在CONFIG\_PM\_OPP之外還啓用了CONFIG\_CPU\_FREQ時纔可用。

dev\_pm\_opp\_free\_cpufreq\_table
:   釋放dev\_pm\_opp\_init\_cpufreq\_table分配的表。
