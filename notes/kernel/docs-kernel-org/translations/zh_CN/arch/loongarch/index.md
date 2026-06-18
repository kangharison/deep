# LoongArch体系结构

> 출처(원문): https://docs.kernel.org/translations/zh_CN/arch/loongarch/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Note

此文件的目的是为让中文读者更容易阅读和理解，而不是作为一个分支。 因此，
如果您对此文件有任何意见或更新，请先尝试更新原始英文文件。
如果您发现本文档与原始文件有任何不同或者有翻译问题，请发建议或者补丁给
该文件的译者，或者请求中文文档维护者和审阅者的帮助。

Original:
:   [LoongArch Architecture](../../../../arch/loongarch/index.html)

Translator:
:   Huacai Chen <[chenhuacai@loongson.cn](mailto:chenhuacai%40loongson.cn)>

# LoongArch体系结构

* [1. LoongArch介绍](introduction.html)
  + [1.1. 寄存器](introduction.html#id1)
  + [1.2. 基础指令集](introduction.html#id6)
  + [1.3. 虚拟内存](introduction.html#id9)
  + [1.4. Loongson与LoongArch的关系](introduction.html#loongsonloongarch)
  + [1.5. 参考文献](introduction.html#loongarch-references-zh-cn)
* [2. 启动 Linux/LoongArch](booting.html)
  + [2.1. BootLoader传递给内核的信息](booting.html#bootloader)
  + [2.2. Linux/LoongArch内核镜像文件头](booting.html#id1)
* [3. LoongArch的IRQ芯片模型（层级关系）](irq-chip-model.html)
  + [3.1. 传统IRQ模型](irq-chip-model.html#irq)
  + [3.2. 扩展IRQ模型](irq-chip-model.html#id1)
  + [3.3. 虚拟扩展IRQ模型](irq-chip-model.html#id2)
  + [3.4. 高级扩展IRQ模型](irq-chip-model.html#id4)
  + [3.5. ACPI相关的定义](irq-chip-model.html#acpi)
  + [3.6. 参考文献](irq-chip-model.html#id5)
* [4. Feature status on loongarch architecture](features.html)
