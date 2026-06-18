# Linux虚拟化支持

> 출처(원문): https://docs.kernel.org/translations/zh_CN/virt/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Note

此文件的目的是为让中文读者更容易阅读和理解，而不是作为一个分支。 因此，
如果您对此文件有任何意见或更新，请先尝试更新原始英文文件。
如果您发现本文档与原始文件有任何不同或者有翻译问题，请发建议或者补丁给
该文件的译者，或者请求中文文档维护者和审阅者的帮助。

Original:
:   [Virtualization Support](../../../virt/index.html)

翻译:
:   司延腾 Yanteng Si <[siyanteng@loongson.cn](mailto:siyanteng%40loongson.cn)>

校译:
:   时奎亮 Alex Shi <[alexs@kernel.org](mailto:alexs%40kernel.org)>

# Linux虚拟化支持

* [半虚拟化操作](paravirt_ops.html)
* [客户机停机轮询机制（Guest halt polling）](guest-halt-polling.html)
  + [模块参数](guest-halt-polling.html#id1)
  + [进一步说明](guest-halt-polling.html#id2)
* [Nitro Enclaves](ne_overview.html)
  + [概述](ne_overview.html#id1)
* [ACRN超级管理器](acrn/index.html)
  + [ACRN超级管理器介绍](acrn/introduction.html)
  + [I/O请求处理](acrn/io-request.html)
  + [ACRN CPUID位域](acrn/cpuid.html)

TODOLIST:

> kvm/index
> uml/user\_mode\_linux\_howto\_v2
