# 核心API文档

> 출처(원문): https://docs.kernel.org/translations/zh_CN/core-api/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Note

此文件的目的是为让中文读者更容易阅读和理解，而不是作为一个分支。 因此，
如果您对此文件有任何意见或更新，请先尝试更新原始英文文件。
如果您发现本文档与原始文件有任何不同或者有翻译问题，请发建议或者补丁给
该文件的译者，或者请求中文文档维护者和审阅者的帮助。

Original:
:   [Core API Documentation](../../../core-api/index.html)

翻译:
:   司延腾 Yanteng Si <[siyanteng@loongson.cn](mailto:siyanteng%40loongson.cn)>

# 核心API文档

这是核心内核API手册的首页。 非常感谢为本手册转换(和编写!)的文档!

## 核心实用程序

本节包含通用的和“核心中的核心”文档。 第一部分是 docbook 时期遗留下
来的大量 kerneldoc 信息；有朝一日，若有人有动力的话，应当把它们拆分
出来。

* [Linux内核API](kernel-api.html)
* [使用printk记录消息](printk-basics.html)
* [如何获得正确的printk格式占位符](printk-formats.html)
* [工作队列](workqueue.html)
* [通用通知机制](watch_queue.html)
* [符号命名空间（Symbol Namespaces）](symbol-namespaces.html)

## 数据结构和低级实用程序

在整个内核中使用的函数库。

* [关于kobjects、ksets和ktypes的一切你没想过需要了解的东西](kobject.html)
* [为内核对象添加引用计数器（krefs）](kref.html)
* [通用关联数组的实现](assoc_array.html)
* [XArray](xarray.html)
* [Linux中的红黑树（rbtree）](rbtree.html)
* [ID分配](idr.html)
* [环形缓冲区](circular-buffers.html)
* [通用基数树/稀疏数组](generic-radix-tree.html)
* [通用的位域打包和解包函数](packing.html)
* [this\_cpu操作](this_cpu_ops.html)
* [Linux中的并查集（Union-Find）](union_find.html)

---

Todolist:

> timekeeping
> errseq

## 并发原语

Linux如何让一切同时发生。 详情请参阅
[Locking](../../../locking/index.html)

* [IRQs](irq/index.html)
* [与atomic\_t相比，refcount\_t的API是这样的](refcount-vs-atomic.html)
* [本地原子操作的语义和行为](local_ops.html)
* [padata并行执行机制](padata.html)

Todolist:

> ../RCU/index

## 低级硬件管理

缓存管理，CPU热插拔管理等。

* [Linux下的缓存和TLB刷新](cachetlb.html)
* [内核中的CPU热拔插](cpu_hotplug.html)
* [Linux通用IRQ处理](genericirq.html)
* [内存热插拔](memory-hotplug.html)
* [内存保护密钥](protection-keys.html)

Todolist:

> memory-hotplug
> cpu\_hotplug
> genericirq

## 内存管理

如何在内核中分配和使用内存。请注意，在
[Memory Management Documentation](../../../mm/index.html) 中有更多的内存管理文档。

* [内存分配指南](memory-allocation.html)
* [非对齐内存访问](unaligned-memory-access.html)
* [内存管理APIs](mm-api.html)
* [genalloc/genpool子系统](genalloc.html)
* [启动时的内存管理](boot-time-mm.html)
* [从FS/IO上下文中使用的GFP掩码](gfp_mask-from-fs-io.html)

Todolist:

> dma-api
> dma-api-howto
> dma-attributes
> dma-isa-lpc
> pin\_user\_pages

## 内核调试的接口

Todolist:

> debug-objects
> tracepoint
> debugging-via-ohci1394

## 其它文档

不适合放在其它地方或尚未归类的文件；

Todolist:

> librs
