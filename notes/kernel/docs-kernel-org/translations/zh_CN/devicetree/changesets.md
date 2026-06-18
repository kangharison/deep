# 设备树变更集

> 출처(원문): https://docs.kernel.org/translations/zh_CN/devicetree/changesets.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Note

此文件的目的是为让中文读者更容易阅读和理解，而不是作为一个分支。 因此，
如果您对此文件有任何意见或更新，请先尝试更新原始英文文件。
如果您发现本文档与原始文件有任何不同或者有翻译问题，请发建议或者补丁给
该文件的译者，或者请求中文文档维护者和审阅者的帮助。

Original:
:   [Devicetree Changesets](../../../devicetree/changesets.html)

翻译:
:   司延腾 Yanteng Si <[siyanteng@loongson.cn](mailto:siyanteng%40loongson.cn)>

校译:

# 设备树变更集

设备树变更集是一种方法，它允许人们以这样一种方式在实时树中使用变化，即要么使用全部的
变化，要么不使用。如果在使用变更集的过程中发生错误，那么树将被回滚到之前的状态。一个
变更集也可以在使用后被删除。

当一个变更集被使用时，所有的改变在发出OF\_RECONFIG通知器之前被一次性使用到树上。这是
为了让接收者在收到通知时看到一个完整的、一致的树的状态。

一个变化集的顺序如下。

1. [`of_changeset_init()`](../../../devicetree/kernel-api.html#c.of_changeset_init "of_changeset_init") - 初始化一个变更集。
2. 一些DT树变化的调用，`of_changeset_attach_node()`, `of_changeset_detach_node()`,
   `of_changeset_add_property()`, of\_changeset\_remove\_property,
   `of_changeset_update_property()`来准备一组变更。此时不会对活动树做任何变更。所有
   的变更操作都记录在of\_changeset的 entries 列表中。
3. [`of_changeset_apply()`](../../../devicetree/kernel-api.html#c.of_changeset_apply "of_changeset_apply") - 将变更使用到树上。要么整个变更集被使用，要么如果有错误，
   树会被恢复到之前的状态。核心通过锁确保正确的顺序。如果需要的话，可以使用一个解锁的
   \_\_of\_changeset\_apply版本。

如果一个成功使用的变更集需要被删除，可以用[`of_changeset_revert()`](../../../devicetree/kernel-api.html#c.of_changeset_revert "of_changeset_revert")来完成。
