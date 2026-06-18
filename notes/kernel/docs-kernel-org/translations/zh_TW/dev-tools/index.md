# 內核開發工具

> 출처(원문): https://docs.kernel.org/translations/zh_TW/dev-tools/index.html
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
:   [Development tools for the kernel](../../../dev-tools/index.html)

Translator:
:   趙軍奎 Bernard Zhao <[bernard@vivo.com](mailto:bernard%40vivo.com)>

# 內核開發工具

本文檔是有關內核開發工具文檔的合集。
目前這些文檔已經整理在一起，不需要再花費額外的精力。
歡迎任何補丁。

有關測試專用工具的簡要概述，參見
[內核測試指南](testing-overview.html)

目錄

* [內核測試指南](testing-overview.html)
  + [編寫和運行測試](testing-overview.html#id2)
  + [代碼覆蓋率工具](testing-overview.html#id3)
  + [動態分析工具](testing-overview.html#id4)
  + [靜態分析工具](testing-overview.html#id5)
* [Traditional Chinese maintainer: Hu Haowen <2023002089@link.tyut.edu.cn>](sparse.html)
* [以下爲正文](sparse.html#id1)
  + [使用 sparse 工具做類型檢查](sparse.html#sparse)
  + [獲取 sparse 工具](sparse.html#id2)
  + [使用 sparse 工具](sparse.html#id3)
* [在Linux內核裏使用gcov做代碼覆蓋率檢查](gcov.html)
  + [準備](gcov.html#id1)
  + [定製化](gcov.html#id2)
  + [相關文件](gcov.html#id3)
  + [針對模塊的統計](gcov.html#id4)
  + [編譯機和測試機分離](gcov.html#id5)
  + [關於編譯器的注意事項](gcov.html#id6)
  + [問題定位](gcov.html#id8)
  + [附錄A：collect\_on\_build.sh](gcov.html#a-collect-on-build-sh)
  + [附錄B：collect\_on\_test.sh](gcov.html#b-collect-on-test-sh)
* [內核地址消毒劑(KASAN)](kasan.html)
  + [概述](kasan.html#id1)
  + [支持](kasan.html#id2)
  + [用法](kasan.html#id6)
  + [實施細則](kasan.html#id9)
  + [影子內存](kasan.html#id13)
  + [對於開發者](kasan.html#id15)
* [通過gdb調試內核和模塊](gdb-kernel-debugging.html)
  + [環境配置要求](gdb-kernel-debugging.html#id1)
  + [設置](gdb-kernel-debugging.html#id2)
  + [使用Linux提供的gdb腳本的示例](gdb-kernel-debugging.html#linuxgdb)
  + [命令和輔助調試功能列表](gdb-kernel-debugging.html#id3)

Todolist:

> * coccinelle
> * kcov
> * ubsan
> * kmemleak
> * kcsan
> * kfence
> * kgdb
> * kselftest
> * kunit/index
