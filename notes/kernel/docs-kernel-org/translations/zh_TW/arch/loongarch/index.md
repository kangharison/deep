# LoongArch體系結構

> 출처(원문): https://docs.kernel.org/translations/zh_TW/arch/loongarch/index.html
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
:   [LoongArch Architecture](../../../../arch/loongarch/index.html)

Translator:
:   Huacai Chen <[chenhuacai@loongson.cn](mailto:chenhuacai%40loongson.cn)>

# LoongArch體系結構

* [1. LoongArch介紹](introduction.html)
  + [1.1. 寄存器](introduction.html#id1)
  + [1.2. 基礎指令集](introduction.html#id6)
  + [1.3. 虛擬內存](introduction.html#id9)
  + [1.4. Loongson與LoongArch的關係](introduction.html#loongsonloongarch)
  + [1.5. 參考文獻](introduction.html#loongarch-references-zh-tw)
* [2. 啓動 Linux/LoongArch](booting.html)
  + [2.1. BootLoader傳遞給內核的信息](booting.html#bootloader)
  + [2.2. Linux/LoongArch內核鏡像文件頭](booting.html#id1)
* [3. LoongArch的IRQ芯片模型（層級關係）](irq-chip-model.html)
  + [3.1. 傳統IRQ模型](irq-chip-model.html#irq)
  + [3.2. 擴展IRQ模型](irq-chip-model.html#id1)
  + [3.3. ACPI相關的定義](irq-chip-model.html#acpi)
  + [3.4. 參考文獻](irq-chip-model.html#id2)
* [4. Feature status on loongarch architecture](features.html)
