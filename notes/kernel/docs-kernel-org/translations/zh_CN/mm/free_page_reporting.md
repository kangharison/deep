# 空闲页报告

> 출처(원문): https://docs.kernel.org/translations/zh_CN/mm/free_page_reporting.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Note

此文件的目的是为让中文读者更容易阅读和理解，而不是作为一个分支。 因此，
如果您对此文件有任何意见或更新，请先尝试更新原始英文文件。
如果您发现本文档与原始文件有任何不同或者有翻译问题，请发建议或者补丁给
该文件的译者，或者请求中文文档维护者和审阅者的帮助。

Original:
:   [Free Page Reporting](../../../mm/free_page_reporting.html)

翻译:
:   司延腾 Yanteng Si <[siyanteng@loongson.cn](mailto:siyanteng%40loongson.cn)>

校译:

# 空闲页报告

空闲页报告是一个API，设备可以通过它来注册接收系统当前未使用的页面列表。这在虚拟
化的情况下是很有用的，客户机能够使用这些数据来通知管理器它不再使用内存中的某些页
面。

对于驱动，通常是气球驱动要使用这个功能，它将分配和初始化一个page\_reporting\_dev\_info
结构体。它要填充的结构体中的字段是用于处理散点列表的 “report” 函数指针。它还必
须保证每次调用该函数时能处理至少相当于PAGE\_REPORTING\_CAPACITY的散点列表条目。
假设没有其他页面报告设备已经注册， 对page\_reporting\_register的调用将向报告框
架注册页面报告接口。

一旦注册，页面报告API将开始向驱动报告成批的页面。API将在接口被注册后2秒开始报告
页面，并在任何足够高的页面被释放之后2秒继续报告。

报告的页面将被存储在传递给报告函数的散列表中，最后一个条目的结束位被设置在条目
nent-1中。 当页面被报告函数处理时，分配器将无法访问它们。一旦报告函数完成，这些
页将被返回到它们所获得的自由区域。

在移除使用空闲页报告的驱动之前，有必要调用page\_reporting\_unregister，以移除
目前被空闲页报告使用的page\_reporting\_dev\_info结构体。这样做将阻止进一步的报
告通过该接口发出。如果另一个驱动或同一驱动被注册，它就有可能恢复前一个驱动在报告
空闲页方面的工作。

Alexander Duyck, 2019年12月04日
