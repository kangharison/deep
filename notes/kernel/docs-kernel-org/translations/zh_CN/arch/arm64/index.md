# ARM64 架构

> 출처(원문): https://docs.kernel.org/translations/zh_CN/arch/arm64/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Note

此文件的目的是为让中文读者更容易阅读和理解，而不是作为一个分支。 因此，
如果您对此文件有任何意见或更新，请先尝试更新原始英文文件。
如果您发现本文档与原始文件有任何不同或者有翻译问题，请发建议或者补丁给
该文件的译者，或者请求中文文档维护者和审阅者的帮助。

Original:
:   [Documentation/arch/arm64/index.rst](../../../../arch/arm64/index.html#arm64-index)

Translator:
:   Bailu Lin <[bailu.lin@vivo.com](mailto:bailu.lin%40vivo.com)>

# ARM64 架构

* [AArch64 Linux 中扩展的活动监控单元](amu.html)
  + [架构总述](amu.html#id1)
  + [基本支持](amu.html#id2)
  + [用户空间访问](amu.html#id3)
  + [虚拟化](amu.html#id4)
* [ARM64中的 HugeTLBpage](hugetlbpage.html)
  + [1) pud/pmd 级别的块映射](hugetlbpage.html#pud-pmd)
  + [2) 使用连续位](hugetlbpage.html#id1)
* [Perf 事件属性](perf.html)
  + [exclude\_user](perf.html#exclude-user)
  + [exclude\_kernel](perf.html#exclude-kernel)
  + [exclude\_hv](perf.html#exclude-hv)
  + [exclude\_host / exclude\_guest](perf.html#exclude-host-exclude-guest)
  + [准确性](perf.html#id1)
* [ARM64 ELF hwcaps](elf_hwcaps.html)
  + [1. 简介](elf_hwcaps.html#id1)
  + [2. Hwcaps 的说明](elf_hwcaps.html#hwcaps)
  + [3. AT\_HWCAP 中揭示的 hwcaps](elf_hwcaps.html#at-hwcap-hwcaps)
  + [4. 未使用的 AT\_HWCAP 位](elf_hwcaps.html#at-hwcap)
