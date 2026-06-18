# 与Linux 内核社区一起工作

> 출처(원문): https://docs.kernel.org/translations/zh_CN/process/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Note

此文件的目的是为让中文读者更容易阅读和理解，而不是作为一个分支。 因此，
如果您对此文件有任何意见或更新，请先尝试更新原始英文文件。
如果您发现本文档与原始文件有任何不同或者有翻译问题，请发建议或者补丁给
该文件的译者，或者请求中文文档维护者和审阅者的帮助。

Original:
:   [Working with the kernel development community](../../../process/index.html)

翻译:
:   Alex Shi <[alex.shi@linux.alibaba.com](mailto:alex.shi%40linux.alibaba.com)>

# 与Linux 内核社区一起工作

你想成为Linux内核开发人员吗？欢迎之至！在学习许多关于内核的技术知识的同时，
了解我们社区的工作方式也很重要。阅读这些文档可以让您以更轻松的、麻烦更少的
方式将更改合并到内核。

以下是每位开发人员都应阅读的基本指南：

* [Linux内核许可规则](license-rules.html)
* [如何参与Linux内核开发](howto.html)
* [贡献者契约行为准则](code-of-conduct.html)
* [Linux内核贡献者契约行为准则解释](code-of-conduct-interpretation.html)
* [内核开发过程指南](development-process.html)
* [提交补丁：如何让你的改动进入内核](submitting-patches.html)
* [程序设计语言](programming-language.html)
* [Linux 内核代码风格](coding-style.html)
* [内核维护者 PGP 指南](maintainer-pgp-guide.html)
* [Linux邮件客户端配置信息](email-clients.html)
* [Linux 内核执行声明](kernel-enforcement-statement.html)
* [内核驱动声明](kernel-driver-statement.html)

TODOLIST:

* handling-regressions
* maintainer-handbooks

安全方面, 请阅读:

* [被限制的硬件问题](embargoed-hardware-issues.html)
* [CVEs](cve.html)
* [安全缺陷](security-bugs.html)

TODOLIST:

* handling-regressions

其它大多数开发人员感兴趣的社区指南：

* [Linux 内核驱动接口](stable-api-nonsense.html)
* [Linux内核管理风格](management-style.html)
* [所有你想知道的事情 - 关于linux稳定版发布](stable-kernel-rules.html)
* [Linux内核补丁提交检查单](submit-checklist.html)
* [研究人员指南](researcher-guidelines.html)

TODOLIST:

* changes
* kernel-docs
* deprecated
* maintainers
* contribution-maturity-model

这些是一些总体性技术指南，由于不大好分类而放在这里：

* [Linux 魔术数](magic-number.html)
* [为什么不应该使用“volatile”类型](volatile-considered-harmful.html)
* [arch/riscv 开发者维护指南](../arch/riscv/patch-acceptance.html)
* [非对齐内存访问](../core-api/unaligned-memory-access.html)

TODOLIST:

* applying-patches
* backporting
* adding-syscalls
* botching-up-ioctls
* clang-format
