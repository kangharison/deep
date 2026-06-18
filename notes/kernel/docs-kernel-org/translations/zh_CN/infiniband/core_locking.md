# infiniband中间层锁

> 출처(원문): https://docs.kernel.org/translations/zh_CN/infiniband/core_locking.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Note

此文件的目的是为让中文读者更容易阅读和理解，而不是作为一个分支。 因此，
如果您对此文件有任何意见或更新，请先尝试更新原始英文文件。
如果您发现本文档与原始文件有任何不同或者有翻译问题，请发建议或者补丁给
该文件的译者，或者请求中文文档维护者和审阅者的帮助。

Original:
:   [InfiniBand Midlayer Locking](../../../infiniband/core_locking.html)

翻译:
:   司延腾 Yanteng Si <[siyanteng@loongson.cn](mailto:siyanteng%40loongson.cn)>

校译:
:   王普宇 Puyu Wang <[realpuyuwang@gmail.com](mailto:realpuyuwang%40gmail.com)>
    时奎亮 Alex Shi <[alexs@kernel.org](mailto:alexs%40kernel.org)>

# infiniband中间层锁

> 本指南试图明确infiniband中间层的锁假设。它描述了对位于中间层以下的低
> 级驱动程序和使用中间层的上层协议的要求。

## 睡眠和中断环境

> 除了以下异常情况，ib\_device结构体中所有方法的低级驱动实现都可以睡眠。
> 这些异常情况是列表中的任意的方法:
>
> > * create\_ah
> > * modify\_ah
> > * query\_ah
> > * destroy\_ah
> > * post\_send
> > * post\_recv
> > * poll\_cq
> > * req\_notify\_cq
> >
> > 他们可能不可以睡眠，而且必须可以从任何上下文中调用。
> >
> > 向上层协议使用者输出的相应函数:
> >
> > * rdma\_create\_ah
> > * rdma\_modify\_ah
> > * rdma\_query\_ah
> > * rdma\_destroy\_ah
> > * ib\_post\_send
> > * ib\_post\_recv
> > * ib\_req\_notify\_cq
> >
> > 因此，在任何情况下都可以安全调用（它们）。
>
> 此外，该函数
>
> > * ib\_dispatch\_event
>
> 被底层驱动用来通过中间层调度异步事件的“A”，也可以从任何上下文中安全调
> 用。

### 可重入性

> 由低级驱动程序导出的ib\_device结构体中的所有方法必须是完全可重入的。
> 即使使用同一对象的多个函数调用被同时运行，低级驱动程序也需要执行所有
> 必要的同步以保持一致性。
>
> IB中间层不执行任何函数调用的序列化。
>
> 因为低级驱动程序是可重入的，所以不要求上层协议使用者任何顺序执行。然
> 而，为了得到合理的结果，可能需要一些顺序。例如，一个使用者可以在多个
> CPU上同时安全地调用`ib_poll_cq()`。然而，不同的`ib_poll_cq()`调用之间
> 的工作完成信息的顺序没有被定义。

### 回调

> 低级驱动程序不得直接从与ib\_device方法调用相同的调用链中执行回调。例
> 如，低级驱动程序不允许从post\_send方法直接调用使用者的完成事件处理程
> 序。相反，低级驱动程序应该推迟这个回调，例如，调度一个tasklet来执行
> 这个回调。
>
> 低层驱动负责确保同一CQ的多个完成事件处理程序不被同时调用。驱动程序必
> 须保证一个给定的CQ的事件处理程序在同一时间只有一个在运行。换句话说，
> 以下情况是不允许的:
>
> ```
>       CPU1                                    CPU2
>
> low-level driver ->
>   consumer CQ event callback:
>     /* ... */
>     ib_req_notify_cq(cq, ...);
>                                       low-level driver ->
>     /* ... */                           consumer CQ event callback:
>                                           /* ... */
>     return from CQ event handler
> ```
>
> 完成事件和异步事件回调的运行环境没有被定义。 根据低级别的驱动程序，它可能是
> 进程上下文、softirq上下文或中断上下文。上层协议使用者可能不会在回调中睡眠。

### 热插拔

> 当一个低级驱动程序调用[`ib_register_device()`](../../../driver-api/infiniband.html#c.ib_register_device "ib_register_device")时，它宣布一个设备已经
> 准备好供使用者使用，所有的初始化必须在这个调用之前完成。设备必须保
> 持可用，直到驱动对[`ib_unregister_device()`](../../../driver-api/infiniband.html#c.ib_unregister_device "ib_unregister_device")的调用返回。
>
> 低级驱动程序必须从进程上下文调用[`ib_register_device()`](../../../driver-api/infiniband.html#c.ib_register_device "ib_register_device")和
> [`ib_unregister_device()`](../../../driver-api/infiniband.html#c.ib_unregister_device "ib_unregister_device")。如果使用者在这些调用中回调到驱动程序，它
> 不能持有任何可能导致死锁的semaphores。
>
> 一旦其结构体ib\_client的add方法被调用，上层协议使用者就可以开始使用
> 一个IB设备。使用者必须在从移除方法返回之前完成所有的清理工作并释放
> 与设备相关的所有资源。
>
> 使用者被允许在其添加和删除方法中睡眠。
