# ARM64中的 HugeTLBpage

> 출처(원문): https://docs.kernel.org/translations/zh_TW/arch/arm64/hugetlbpage.html
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
:   [Documentation/arch/arm64/hugetlbpage.rst](../../../../arch/arm64/hugetlbpage.html#hugetlbpage-index)

Translator: Bailu Lin <[bailu.lin@vivo.com](mailto:bailu.lin%40vivo.com)>
:   Hu Haowen <[2023002089@link.tyut.edu.cn](mailto:2023002089%40link.tyut.edu.cn)>

# ARM64中的 HugeTLBpage

大頁依靠有效利用 TLBs 來提高地址翻譯的性能。這取決於以下
兩點 -

> * 大頁的大小
> * TLBs 支持的條目大小

ARM64 接口支持2種大頁方式。

## 1) pud/pmd 級別的塊映射

這是常規大頁，他們的 pmd 或 pud 頁面表條目指向一個內存塊。
不管 TLB 中支持的條目大小如何，塊映射可以減少翻譯大頁地址
所需遍歷的頁表深度。

## 2) 使用連續位

架構中轉換頁表條目(D4.5.3, ARM DDI 0487C.a)中提供一個連續
位告訴 MMU 這個條目是一個連續條目集的一員，它可以被緩存在單
個 TLB 條目中。

在 Linux 中連續位用來增加 pmd 和 pte(最後一級)級別映射的大
小。受支持的連續頁表條目數量因頁面大小和頁表級別而異。

支持以下大頁尺寸配置 -

> |  | CONT PTE | PMD | CONT PMD | PUD |
> | --- | --- | --- | --- | --- |
> | 4K: | 64K | 2M | 32M | 1G |
> | 16K: | 2M | 32M | 1G |  |
> | 64K: | 2M | 512M | 16G |  |
