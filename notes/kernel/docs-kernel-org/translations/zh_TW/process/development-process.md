# 內核開發過程指南

> 출처(원문): https://docs.kernel.org/translations/zh_TW/process/development-process.html
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
:   [Documentation/process/development-process.rst](../../../process/development-process.html#development-process-main)

Translator:
:   Alex Shi <[alex.shi@linux.alibaba.com](mailto:alex.shi%40linux.alibaba.com)>
    Hu Haowen <[2023002089@link.tyut.edu.cn](mailto:2023002089%40link.tyut.edu.cn)>

# 內核開發過程指南

本文檔的目的是幫助開發人員（及其經理）以最小的挫折感與開發社區合作。它試圖記錄這個社區如何以一種不熟悉Linux內核開發（或者實際上是自由軟體開發）的人可以訪問的方式工作。雖然這裡有一些技術資料，但這是一個面向過程的討論，不需要深入了解內核編程就可以理解。

內容

* [1. 引言](1.Intro.html)
  + [1.1. 內容提要](1.Intro.html#id2)
  + [1.2. 這個文檔是關於什麼的](1.Intro.html#id3)
  + [1.3. 致謝](1.Intro.html#id4)
  + [1.4. 代碼進入主線的重要性](1.Intro.html#id5)
  + [1.5. 許可](1.Intro.html#id6)
* [2. 開發流程如何進行](2.Process.html)
  + [2.1. 總覽](2.Process.html#id2)
  + [2.2. 補丁的生命週期](2.Process.html#id3)
  + [2.3. 補丁如何進入內核](2.Process.html#id4)
  + [2.4. Next 樹](2.Process.html#next)
  + [2.5. Staging 樹](2.Process.html#staging)
  + [2.6. 工具](2.Process.html#id5)
  + [2.7. 郵件列表](2.Process.html#id6)
  + [2.8. 開始內核開發](2.Process.html#id7)
* [3. 早期規劃](3.Early-stage.html)
  + [3.1. 搞清問題](3.Early-stage.html#id2)
  + [3.2. 早期討論](3.Early-stage.html#id3)
  + [3.3. 找誰交流？](3.Early-stage.html#id4)
  + [3.4. 何時郵寄？](3.Early-stage.html#id5)
  + [3.5. 獲得官方認可](3.Early-stage.html#id6)
* [4. 使代碼正確](4.Coding.html)
  + [4.1. 陷阱](4.Coding.html#id2)
  + [4.2. 代碼檢查工具](4.Coding.html#id8)
  + [4.3. 文檔](4.Coding.html#id9)
  + [4.4. 內部API更改](4.Coding.html#api)
* [5. 發佈補丁](5.Posting.html)
  + [5.1. 何時寄送](5.Posting.html#id2)
  + [5.2. 創建補丁之前](5.Posting.html#id3)
  + [5.3. 補丁準備](5.Posting.html#id4)
  + [5.4. 補丁格式和更改日誌](5.Posting.html#id5)
  + [5.5. 寄送補丁](5.Posting.html#id6)
* [6. 跟進](6.Followthrough.html)
  + [6.1. 與審閱者合作](6.Followthrough.html#id2)
  + [6.2. 接下來會發生什麼](6.Followthrough.html#id3)
  + [6.3. 其他可能發生的事情](6.Followthrough.html#id4)
* [7. 高級主題](7.AdvancedTopics.html)
  + [7.1. 使用Git管理補丁](7.AdvancedTopics.html#git)
  + [7.2. 審閱補丁](7.AdvancedTopics.html#id2)
* [8. 更多信息](8.Conclusion.html)
* [9. 結論](8.Conclusion.html#id2)

本文檔的目的是幫助開發人員（及其經理）以最小的挫折感與開發社區合作。它試圖記錄這個社區如何以一種不熟悉Linux內核開發（或者實際上是自由軟件開發）的人可以訪問的方式工作。雖然這裏有一些技術資料，但這是一個面向過程的討論，不需要深入瞭解內核編程就可以理解。
