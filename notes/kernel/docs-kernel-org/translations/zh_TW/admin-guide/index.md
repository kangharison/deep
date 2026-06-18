# Linux 內核用戶和管理員指南

> 출처(원문): https://docs.kernel.org/translations/zh_TW/admin-guide/index.html
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
:   [The Linux kernel user’s and administrator’s guide](../../../admin-guide/index.html)

Translator:
:   Alex Shi <[alex.shi@linux.alibaba.com](mailto:alex.shi%40linux.alibaba.com)>
    胡皓文 Hu Haowen <[2023002089@link.tyut.edu.cn](mailto:2023002089%40link.tyut.edu.cn)>

# Linux 內核用戶和管理員指南

下面是一組隨時間添加到內核中的面向用戶的文檔的集合。到目前爲止，還沒有一個
整體的順序或組織 - 這些材料不是一個單一的，連貫的文件！幸運的話，情況會隨着
時間的推移而迅速改善。

這個初始部分包含總體信息，包括描述內核的README， 關於內核參數的文檔等。

* [Linux內核6.x版本 <http://kernel.org/>](README.html)

Todolist:

* kernel-parameters
* devices
* sysctl/index

本節介紹CPU漏洞及其緩解措施。

Todolist:

* hw-vuln/index

下面的一組文檔，針對的是試圖跟蹤問題和bug的用戶。

* [報告問題](reporting-issues.html)
* [報告迴歸問題](reporting-regressions.html)
* [安全缺陷](security-bugs.html)
* [追蹤缺陷](bug-hunting.html)
* [二分（bisect）缺陷](bug-bisect.html)
* [受污染的內核](tainted-kernels.html)
* [解釋“No working init found.”啓動掛起消息](init.html)

Todolist:

* ramoops
* dynamic-debug-howto
* kdump/index
* perf/index

這是應用程序開發人員感興趣的章節的開始。可以在這裏找到涵蓋內核ABI各個
方面的文檔。

Todolist:

* sysfs-rules

本手冊的其餘部分包括各種指南，介紹如何根據您的喜好配置內核的特定行爲。

* [引導配置](bootconfig.html)
* [清除 WARN\_ONCE](clearing-warn-once.html)
* [CPU 負載](cpu-load.html)
* [如何通過sysfs將CPU拓撲導出](cputopology.html)
* [Softlockup與hardlockup檢測機制(又名:nmi\_watchdog)](lockup-watchdogs.html)
* [Unicode（統一碼）支持](unicode.html)
* [Linux 魔法系統請求鍵駭客](sysrq.html)
* [內存管理](mm/index.html)

Todolist:

* acpi/index
* aoe/index
* auxdisplay/index
* bcache
* binderfs
* binfmt-misc
* blockdev/index
* braille-console
* btmrvl
* cgroup-v1/index
* cgroup-v2
* cifs/index
* dell\_rbu
* device-mapper/index
* edid
* efi-stub
* ext4
* nfs/index
* gpio/index
* highuid
* hw\_random
* initrd
* iostats
* java
* jfs
* kernel-per-CPU-kthreads
* laptops/index
* lcd-panel-cgram
* ldm
* LSM/index
* md
* media/index
* module-signing
* mono
* namespaces/index
* numastat
* parport
* perf-security
* pm/index
* pnp
* rapidio
* ras
* rtc
* serial-console
* svga
* thunderbolt
* ufs
* vga-softcursor
* video-output
* xfs
