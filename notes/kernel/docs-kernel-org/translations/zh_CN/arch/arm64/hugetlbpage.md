# ARM64中的 HugeTLBpage

> 출처(원문): https://docs.kernel.org/translations/zh_CN/arch/arm64/hugetlbpage.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Note

此文件的目的是为让中文读者更容易阅读和理解，而不是作为一个分支。 因此，
如果您对此文件有任何意见或更新，请先尝试更新原始英文文件。
如果您发现本文档与原始文件有任何不同或者有翻译问题，请发建议或者补丁给
该文件的译者，或者请求中文文档维护者和审阅者的帮助。

Original:
:   [Documentation/arch/arm64/hugetlbpage.rst](../../../../arch/arm64/hugetlbpage.html#hugetlbpage-index)

Translator: Bailu Lin <[bailu.lin@vivo.com](mailto:bailu.lin%40vivo.com)>

# ARM64中的 HugeTLBpage

大页依靠有效利用 TLBs 来提高地址翻译的性能。这取决于以下
两点 -

> * 大页的大小
> * TLBs 支持的条目大小

ARM64 接口支持2种大页方式。

## 1) pud/pmd 级别的块映射

这是常规大页，他们的 pmd 或 pud 页面表条目指向一个内存块。
不管 TLB 中支持的条目大小如何，块映射可以减少翻译大页地址
所需遍历的页表深度。

## 2) 使用连续位

架构中转换页表条目(D4.5.3, ARM DDI 0487C.a)中提供一个连续
位告诉 MMU 这个条目是一个连续条目集的一员，它可以被缓存在单
个 TLB 条目中。

在 Linux 中连续位用来增加 pmd 和 pte(最后一级)级别映射的大
小。受支持的连续页表条目数量因页面大小和页表级别而异。

支持以下大页尺寸配置 -

> |  | CONT PTE | PMD | CONT PMD | PUD |
> | --- | --- | --- | --- | --- |
> | 4K: | 64K | 2M | 32M | 1G |
> | 16K: | 2M | 32M | 1G |  |
> | 64K: | 2M | 512M | 16G |  |
