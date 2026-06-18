# Linux CPUFreq - Linux(TM)內核中的CPU頻率和電壓升降代碼

> 출처(원문): https://docs.kernel.org/translations/zh_TW/cpu-freq/index.html
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
:   [CPUFreq - CPU frequency and voltage scaling code in the Linux(TM) kernel](../../../cpu-freq/index.html)

翻譯:
:   司延騰 Yanteng Si <[siyanteng@loongson.cn](mailto:siyanteng%40loongson.cn)>

# Linux CPUFreq - Linux(TM)內核中的CPU頻率和電壓升降代碼

Author: Dominik Brodowski <[linux@brodo.de](mailto:linux%40brodo.de)>

> 時鐘升降允許你在運行中改變CPU的時鐘速度。這是一個很好的節省電池電量的方法，因爲時
> 鐘速度越低，CPU消耗的電量越少。

* [CPUFreq核心和CPUFreq通知器的通用說明](core.html)
* [如何實現一個新的CPUFreq處理器驅動程序？](cpu-drivers.html)
* [sysfs CPUFreq Stats的一般說明](cpufreq-stats.html)

## 郵件列表

這裏有一個 CPU 頻率變化的 CVS 提交和通用列表，您可以在這裏報告bug、問題或提交補丁。要發
布消息，請發送電子郵件到 [linux-pm@vger.kernel.org](mailto:linux-pm%40vger.kernel.org)。

## 鏈接

FTP檔案:
\* <ftp://ftp.linux.org.uk/pub/linux/cpufreq/>

如何訪問CVS倉庫:
\* <http://cvs.arm.linux.org.uk/>

CPUFreq郵件列表:
\* <http://vger.kernel.org/vger-lists.html#linux-pm>

SA-1100的時鐘和電壓標度:
\* <http://www.lartmaker.nl/projects/scaling>
