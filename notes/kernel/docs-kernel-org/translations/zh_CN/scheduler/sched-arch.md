# 架构特定代码的CPU调度器实现提示

> 출처(원문): https://docs.kernel.org/translations/zh_CN/scheduler/sched-arch.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Note

此文件的目的是为让中文读者更容易阅读和理解，而不是作为一个分支。 因此，
如果您对此文件有任何意见或更新，请先尝试更新原始英文文件。
如果您发现本文档与原始文件有任何不同或者有翻译问题，请发建议或者补丁给
该文件的译者，或者请求中文文档维护者和审阅者的帮助。

Original:
:   [CPU Scheduler implementation hints for architecture specific code](../../../scheduler/sched-arch.html)

翻译:
:   司延腾 Yanteng Si <[siyanteng@loongson.cn](mailto:siyanteng%40loongson.cn)>

校译:

# 架构特定代码的CPU调度器实现提示

> Nick Piggin, 2005

## 上下文切换

1. 运行队列锁
默认情况下，switch\_to arch函数在调用时锁定了运行队列。这通常不是一个问题，除非
switch\_to可能需要获取运行队列锁。这通常是由于上下文切换中的唤醒操作造成的。

为了要求调度器在运行队列解锁的情况下调用switch\_to，你必须在头文件
中`#define \_\_ARCH\_WANT\_UNLOCKED\_CTXSW`(通常是定义switch\_to的那个文件）。

在CONFIG\_SMP的情况下，解锁的上下文切换对核心调度器的实现只带来了非常小的性能损
失。

## CPU空转

你的cpu\_idle程序需要遵守以下规则：

1. 现在抢占应该在空闲的例程上禁用。应该只在调用`schedule()`时启用，然后再禁用。
2. need\_resched/TIF\_NEED\_RESCHED 只会被设置，并且在运行任务调用 `schedule()`
   之前永远不会被清除。空闲线程只需要查询need\_resched，并且永远不会设置或清除它。
3. 当cpu\_idle发现（`need_resched()` == ‘true’），它应该调用`schedule()`。否则
   它不应该调用`schedule()`。
4. 在检查need\_resched时，唯一需要禁用中断的情况是，我们要让处理器休眠到下一个中
   断（这并不对need\_resched提供任何保护，它可以防止丢失一个中断）:

   > 4a. 这种睡眠类型的常见问题似乎是:
   >
   > ```
   > local_irq_disable();
   > if (!need_resched()) {
   >         local_irq_enable();
   >         *** resched interrupt arrives here ***
   >         __asm__("sleep until next interrupt");
   > }
   > ```
5. 当need\_resched变为高电平时，TIF\_POLLING\_NRFLAG可以由不需要中断来唤醒它们
   的空闲程序设置。换句话说，它们必须定期轮询need\_resched，尽管做一些后台工作或
   进入低CPU优先级可能是合理的。

   > * 5a. 如果TIF\_POLLING\_NRFLAG被设置，而我们确实决定进入一个中断睡眠，那
   >   :   么需要清除它，然后发出一个内存屏障（接着测试need\_resched，禁用中断，如3中解释）。

arch/x86/kernel/process.c有轮询和睡眠空闲函数的例子。

## 可能出现的arch/问题

我发现的可能的arch问题（并试图解决或没有解决）。:

sparc - 在这一点上，IRQ是开着的（？），把local\_irq\_save改为\_disable。
:   * 待办事项: 需要第二个CPU来禁用抢占 (参考 #1)
