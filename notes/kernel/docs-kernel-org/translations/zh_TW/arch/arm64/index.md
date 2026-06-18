# ARM64 架構

> 출처(원문): https://docs.kernel.org/translations/zh_TW/arch/arm64/index.html
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
:   [Documentation/arch/arm64/index.rst](../../../../arch/arm64/index.html#arm64-index)

Translator:
:   Bailu Lin <[bailu.lin@vivo.com](mailto:bailu.lin%40vivo.com)>
    Hu Haowen <[2023002089@link.tyut.edu.cn](mailto:2023002089%40link.tyut.edu.cn)>

# ARM64 架構

* [AArch64 Linux 中擴展的活動監控單元](amu.html)
  + [架構總述](amu.html#id1)
  + [基本支持](amu.html#id2)
  + [用戶空間訪問](amu.html#id3)
  + [虛擬化](amu.html#id4)
* [ARM64中的 HugeTLBpage](hugetlbpage.html)
  + [1) pud/pmd 級別的塊映射](hugetlbpage.html#pud-pmd)
  + [2) 使用連續位](hugetlbpage.html#id1)
* [Perf 事件屬性](perf.html)
  + [exclude\_user](perf.html#exclude-user)
  + [exclude\_kernel](perf.html#exclude-kernel)
  + [exclude\_hv](perf.html#exclude-hv)
  + [exclude\_host / exclude\_guest](perf.html#exclude-host-exclude-guest)
  + [準確性](perf.html#id1)
* [ARM64 ELF hwcaps](elf_hwcaps.html)
  + [1. 簡介](elf_hwcaps.html#id1)
  + [2. Hwcaps 的說明](elf_hwcaps.html#hwcaps)
  + [3. AT\_HWCAP 中揭示的 hwcaps](elf_hwcaps.html#at-hwcap-hwcaps)
  + [4. 未使用的 AT\_HWCAP 位](elf_hwcaps.html#at-hwcap)
