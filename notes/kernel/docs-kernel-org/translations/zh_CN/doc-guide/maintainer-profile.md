# 文档子系统维护人员条目概述

> 출처(원문): https://docs.kernel.org/translations/zh_CN/doc-guide/maintainer-profile.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Note

此文件的目的是为让中文读者更容易阅读和理解，而不是作为一个分支。 因此，
如果您对此文件有任何意见或更新，请先尝试更新原始英文文件。
如果您发现本文档与原始文件有任何不同或者有翻译问题，请发建议或者补丁给
该文件的译者，或者请求中文文档维护者和审阅者的帮助。

Original:
:   [Documentation subsystem maintainer entry profile](../../../doc-guide/maintainer-profile.html)

译者:
:   吴想成 Wu XiangCheng <[bobwxc@email.cn](mailto:bobwxc%40email.cn)>

# 文档子系统维护人员条目概述

文档“子系统”是内核文档和相关基础设施的中心协调点。它涵盖了 Documentation/ 下
的文件层级（Documentation/devicetree 除外）、scripts/ 下的各种实用程序，并且
在某些情况下的也包括 LICENSES/ 。

不过值得注意的是，这个子系统的边界比通常更加模糊。许多其他子系统维护人员需要
保持对 Documentation/ 某些部分的控制，以便于可以更自由地做更改。除此之外，
许多内核文档都以kernel-doc注释的形式出现在源代码中；这些注释通常（但不总是）
由相关的子系统维护人员维护。

文档子系统的邮件列表是<[linux-doc@vger.kernel.org](mailto:linux-doc%40vger.kernel.org)>。
补丁应尽量针对docs-next树。

## 提交检查单补遗

在进行文档更改时，您应当构建文档以测试，并确保没有引入新的错误或警告。生成
HTML文档并查看结果将有助于避免对文档渲染结果的不必要争执。

## 开发周期的关键节点

补丁可以随时发送，但在合并窗口期间，响应将比通常慢。文档树往往在合并窗口打开
之前关闭得比较晚，因为文档补丁导致回归的风险很小。

## 审阅节奏

我（译注：指Jonathan Corbet <[corbet@lwn.net](mailto:corbet%40lwn.net)>）是文档子系统的唯一维护者，我在
自己的时间里完成这项工作，所以对补丁的响应有时会很慢。当补丁被合并时（或当我
决定拒绝合并补丁时），我都会发送通知。如果您在发送补丁后一周内没有收到回复，
请不要犹豫，发送提醒就好。
