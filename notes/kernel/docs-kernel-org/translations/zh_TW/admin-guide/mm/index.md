# 內存管理

> 출처(원문): https://docs.kernel.org/translations/zh_TW/admin-guide/mm/index.html
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
:   [Memory Management](../../../../admin-guide/mm/index.html)

翻譯:
:   徐鑫 xu xin <[xu.xin16@zte.com.cn](mailto:xu.xin16%40zte.com.cn)>

# 內存管理

Linux內存管理子系統，顧名思義，是負責系統中的內存管理。它包括了虛擬內存與請求
分頁的實現，內核內部結構和用戶空間程序的內存分配、將文件映射到進程地址空間以
及許多其他很酷的事情。

Linux內存管理是一個具有許多可配置設置的複雜系統, 且這些設置中的大多數都可以通
過 `/proc` 文件系統獲得，並且可以使用 `sysctl` 進行查詢和調整。這些API接
口被描述在Documentation/admin-guide/sysctl/vm.rst文件和 [man 5 proc](http://man7.org/linux/man-pages/man5/proc.5.html) 中。

Linux內存管理有它自己的術語，如果你還不熟悉它，請考慮閱讀下面參考：
[Concepts overview](../../../../admin-guide/mm/concepts.html).

在此目錄下，我們詳細描述瞭如何與Linux內存管理中的各種機制交互。

* [監測數據訪問](damon/index.html)
* [內核同頁合併](ksm.html)

Todolist:
\* concepts
\* cma\_debugfs
\* hugetlbpage
\* idle\_page\_tracking
\* memory-hotplug
\* nommu-mmap
\* numa\_memory\_policy
\* numaperf
\* pagemap
\* soft-dirty
\* swap\_numa
\* transhuge
\* userfaultfd
\* zswap
