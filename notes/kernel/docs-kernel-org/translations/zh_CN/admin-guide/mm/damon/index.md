# 监测数据访问

> 출처(원문): https://docs.kernel.org/translations/zh_CN/admin-guide/mm/damon/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Note

此文件的目的是为让中文读者更容易阅读和理解，而不是作为一个分支。 因此，
如果您对此文件有任何意见或更新，请先尝试更新原始英文文件。
如果您发现本文档与原始文件有任何不同或者有翻译问题，请发建议或者补丁给
该文件的译者，或者请求中文文档维护者和审阅者的帮助。

Original:
:   [DAMON: Data Access MONitoring and Access-aware System Operations](../../../../../admin-guide/mm/damon/index.html)

翻译:
:   司延腾 Yanteng Si <[siyanteng@loongson.cn](mailto:siyanteng%40loongson.cn)>

校译:

# 监测数据访问

[DAMON](../../../../../mm/damon/index.html) 允许轻量级的数据访问监测。使用DAMON，
用户可以分析他们系统的内存访问模式，并优化它们。

* [入门指南](start.html)
  + [前提条件](start.html#id2)
  + [记录数据访问模式](start.html#id5)
  + [将记录的模式可视化](start.html#id6)
  + [数据访问模式感知的内存管理](start.html#id7)
* [详细用法](usage.html)
  + [sysfs接口](usage.html#sysfs)
  + [监测结果的监测点](usage.html#id7)
* [基于DAMON的回收](reclaim.html)
  + [哪些地方需要主动回收？](reclaim.html#id1)
  + [它是如何工作的？](reclaim.html#id5)
  + [接口: 模块参数](reclaim.html#id6)
  + [例子](reclaim.html#id7)
* [基于DAMON的LRU排序](lru_sort.html)
  + [哪里需要主动的LRU排序](lru_sort.html#lru)
  + [这是如何工作的](lru_sort.html#id1)
  + [接口：模块参数](lru_sort.html#id2)
  + [Example](lru_sort.html#example)
