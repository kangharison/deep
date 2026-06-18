# 監測數據訪問

> 출처(원문): https://docs.kernel.org/translations/zh_TW/admin-guide/mm/damon/index.html
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
:   [DAMON: Data Access MONitoring and Access-aware System Operations](../../../../../admin-guide/mm/damon/index.html)

翻譯:
:   司延騰 Yanteng Si <[siyanteng@loongson.cn](mailto:siyanteng%40loongson.cn)>

校譯:

# 監測數據訪問

[DAMON](../../../../../mm/damon/index.html) 允許輕量級的數據訪問監測。使用DAMON，
用戶可以分析他們系統的內存訪問模式，並優化它們。

* [入門指南](start.html)
  + [前提條件](start.html#id2)
  + [記錄數據訪問模式](start.html#id5)
  + [將記錄的模式可視化](start.html#id6)
  + [數據訪問模式感知的內存管理](start.html#id7)
* [詳細用法](usage.html)
  + [sysfs接口](usage.html#sysfs)
  + [監測結果的監測點](usage.html#id7)
* [基於DAMON的回收](reclaim.html)
  + [哪些地方需要主動回收？](reclaim.html#id1)
  + [它是如何工作的？](reclaim.html#id5)
  + [接口: 模塊參數](reclaim.html#id6)
  + [例子](reclaim.html#id7)
* [基於DAMON的LRU排序](lru_sort.html)
  + [哪裏需要主動的LRU排序](lru_sort.html#lru)
  + [這是如何工作的](lru_sort.html#id1)
  + [接口：模塊參數](lru_sort.html#id2)
  + [Example](lru_sort.html#example)
