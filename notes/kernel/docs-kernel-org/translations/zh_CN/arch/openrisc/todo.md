# 待办事项

> 출처(원문): https://docs.kernel.org/translations/zh_CN/arch/openrisc/todo.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Note

此文件的目的是为让中文读者更容易阅读和理解，而不是作为一个分支。 因此，
如果您对此文件有任何意见或更新，请先尝试更新原始英文文件。
如果您发现本文档与原始文件有任何不同或者有翻译问题，请发建议或者补丁给
该文件的译者，或者请求中文文档维护者和审阅者的帮助。

Original:
:   [TODO](../../../../arch/openrisc/todo.html)

翻译:
:   司延腾 Yanteng Si <[siyanteng@loongson.cn](mailto:siyanteng%40loongson.cn)>

# 待办事项

OpenRISC Linux的移植已经完全投入使用，并且从 2.6.35 开始就一直在上游同步。
然而，还有一些剩余的项目需要在未来几个月内完成。 下面是一个即将进行调查的已知
不尽完美的项目列表，即我们的待办事项列表。

* 实现其余的DMA API……dma\_map\_sg等。
* 完成重命名清理工作……代码中提到了or32，这是架构的一个老名字。 我们
  已经确定的名字是or1k，这个改变正在以缓慢积累的方式进行。 目前，or32相当
  于or1k。
