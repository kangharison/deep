# sysfs CPUFreq Stats的一般說明

> 출처(원문): https://docs.kernel.org/translations/zh_TW/cpu-freq/cpufreq-stats.html
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
:   [General Description of sysfs CPUFreq Stats](../../../cpu-freq/cpufreq-stats.html)

翻譯:
:   司延騰 Yanteng Si <[siyanteng@loongson.cn](mailto:siyanteng%40loongson.cn)>

校譯:
:   唐藝舟 Tang Yizhou <[tangyeechou@gmail.com](mailto:tangyeechou%40gmail.com)>

# sysfs CPUFreq Stats的一般說明

爲使用者準備的信息

作者: Venkatesh Pallipadi <[venkatesh.pallipadi@intel.com](mailto:venkatesh.pallipadi%40intel.com)>

## 1. 簡介

cpufreq-stats是一種爲每個CPU提供CPU頻率統計的驅動。
這些統計數據以/sysfs中一系列只讀接口的形式呈現。cpufreq-stats接口（若已配置）將爲每個CPU生成
/sysfs（<sysfs root>/devices/system/cpu/cpuX/cpufreq/stats/）中cpufreq目錄下的stats目錄。
各項統計數據將在stats目錄下形成對應的只讀文件。

此驅動是以獨立於任何可能運行在你所用CPU上的特定cpufreq\_driver的方式設計的。因此，它將能和任何
cpufreq\_driver協同工作。

## 2. 已提供的統計數據(有例子)

cpufreq stats提供了以下統計數據（在下面詳細解釋）。

* time\_in\_state
* total\_trans
* trans\_table

所有統計數據來自以下時間範圍：從統計驅動被加載的時間（或統計數據被重置的時間）開始，到某一統計數據被讀取的時間爲止。
顯然，統計驅動不會保存它被加載之前的任何頻率轉換信息。

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

只寫屬性，可用於重置統計計數器。這對於評估不同調節器的系統行爲非常有用，且無需重啓。

* **time\_in\_state**

此文件給出了在本CPU支持的每個頻率上分別花費的時間。cat輸出的每一行都是一個”<frequency>
<time>”對，表示這個CPU在<frequency>上花費了<time>個usertime單位的時間。輸出的每一行對應
一個CPU支持的頻率。這裏usertime單位是10mS（類似於/proc導出的其它時間）。

```
<mysystem>:/sys/devices/system/cpu/cpu0/cpufreq/stats # cat time_in_state
3600000 2089
3400000 136
3200000 34
3000000 67
2800000 172488
```

* **total\_trans**

此文件給出了這個CPU頻率轉換的總次數。cat的輸出是一個計數值，它就是頻率轉換的總次數。

```
<mysystem>:/sys/devices/system/cpu/cpu0/cpufreq/stats # cat total_trans
20
```

* **trans\_table**

本文件提供所有CPU頻率轉換的細粒度信息。這裏的cat輸出是一個二維矩陣，其中一個條目<i, j>（第
i行，第j列）代表從Freq\_i到Freq\_j的轉換次數。Freq\_i行和Freq\_j列遵循驅動最初提供給cpufreq
核心的頻率表的排列順序，因此可以已排序（升序或降序）或未排序。這裏的輸出也包含了實際
頻率值，分別按行和按列顯示，以便更好地閱讀。

如果轉換表大於PAGE\_SIZE，讀取時將返回一個-EFBIG錯誤。

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

按以下方式在你的內核中配置cpufreq-stats:

```
Config Main Menu
        Power management options (ACPI, APM)  --->
                CPU Frequency scaling  --->
                        [*] CPU Frequency scaling
                        [*]   CPU frequency translation statistics
```

“CPU Frequency scaling” (CONFIG\_CPU\_FREQ) 應該被啓用，以支持配置cpufreq-stats。

“CPU frequency translation statistics” (CONFIG\_CPU\_FREQ\_STAT)提供了包括
time\_in\_state、total\_trans和trans\_table的統計數據。

一旦啓用了這個選項，並且你的CPU支持cpufrequency，你就可以在/sysfs中看到CPU頻率統計。
