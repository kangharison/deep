# 链路电源管理策略

> 출처(원문): https://docs.kernel.org/translations/zh_CN/scsi/link_power_management_policy.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Note

此文件的目的是为让中文读者更容易阅读和理解，而不是作为一个分支。 因此，
如果您对此文件有任何意见或更新，请先尝试更新原始英文文件。
如果您发现本文档与原始文件有任何不同或者有翻译问题，请发建议或者补丁给
该文件的译者，或者请求中文文档维护者和审阅者的帮助。

Original:
:   [Link Power Managent Policy](../../../scsi/link_power_management_policy.html)

翻译:
:   郝栋栋 doubled <[doubled@leap-io-kernel.com](mailto:doubled%40leap-io-kernel.com)>

校译:

# 链路电源管理策略

该参数允许用户设置链路（接口）的电源管理模式。
共计三类可选项：

| 选项 | 作用 |
| --- | --- |
| min\_power | 指示控制器在可能的情况下尽量使链路处于最低功耗。 这可能会牺牲一定的性能，因为从低功耗状态恢复时会增加延迟。 |
| max\_performance | 通常，这意味着不进行电源管理。指示 控制器优先考虑性能而非电源管理。 |
| medium\_power | 指示控制器在可能的情况下进入较低功耗状态， 而非最低功耗状态，从而改善min\_power模式下的延迟。 |
