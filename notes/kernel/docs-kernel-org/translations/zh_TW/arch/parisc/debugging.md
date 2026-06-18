# 調試PA-RISC

> 출처(원문): https://docs.kernel.org/translations/zh_TW/arch/parisc/debugging.html
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
:   [PA-RISC Debugging](../../../../arch/parisc/debugging.html)

翻譯:
:   司延騰 Yanteng Si <[siyanteng@loongson.cn](mailto:siyanteng%40loongson.cn)>

# 調試PA-RISC

好吧，這裏有一些關於調試linux/parisc的較底層部分的信息。

## 1. 絕對地址

很多彙編代碼目前運行在實模式下，這意味着會使用絕對地址，而不是像內核其他
部分那樣使用虛擬地址。要將絕對地址轉換爲虛擬地址，你可以在System.map中查
找，添加\_\_PAGE\_OFFSET（目前是0x10000000）。

## 2. HPMCs

當實模式的代碼試圖訪問不存在的內存時，會出現HPMC（high priority machine
check）而不是內核oops。若要調試HPMC，請嘗試找到系統響應程序/請求程序地址。
系統請求程序地址應該與（某）處理器的HPA（I/O範圍內的高地址）相匹配；系統響應程
序地址是實模式代碼試圖訪問的地址。

系統響應程序地址的典型值是大於\_\_PAGE\_OFFSET （0x10000000）的地址，這意味着
在實模式試圖訪問它之前，虛擬地址沒有被翻譯成物理地址。

## 3. 有趣的Q位

某些非常關鍵的代碼必須清除PSW中的Q位。當Q位被清除時，CPU不會更新中斷處理
程序所讀取的寄存器，以找出機器被中斷的位置——所以如果你在清除Q位的指令和再
次設置Q位的RFI之間遇到中斷，你不知道它到底發生在哪裏。如果你幸運的話，IAOQ
會指向清除Q位的指令，如果你不幸運的話，它會指向任何地方。通常Q位的問題會
表現爲無法解釋的系統掛起或物理內存越界。
