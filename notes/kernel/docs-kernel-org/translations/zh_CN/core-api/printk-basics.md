# 使用printk记录消息

> 출처(원문): https://docs.kernel.org/translations/zh_CN/core-api/printk-basics.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Note

此文件的目的是为让中文读者更容易阅读和理解，而不是作为一个分支。 因此，
如果您对此文件有任何意见或更新，请先尝试更新原始英文文件。
如果您发现本文档与原始文件有任何不同或者有翻译问题，请发建议或者补丁给
该文件的译者，或者请求中文文档维护者和审阅者的帮助。

Original:
:   [Message logging with printk](../../../core-api/printk-basics.html)

翻译:
:   司延腾 Yanteng Si <[siyanteng@loongson.cn](mailto:siyanteng%40loongson.cn)>
    周彬彬 Binbin Zhou <[zhoubinbin@loongson.cn](mailto:zhoubinbin%40loongson.cn)>

# 使用printk记录消息

[`printk()`](../../../core-api/printk-basics.html#c.printk "printk")是Linux内核中最广为人知的函数之一。它是我们打印消息的标准工具，通常也是追踪和调试
的最基本方法。如果你熟悉printf(3)，你就能够知道[`printk()`](../../../core-api/printk-basics.html#c.printk "printk")是基于它的，尽管它在功能上有一些不
同之处:

> * [`printk()`](../../../core-api/printk-basics.html#c.printk "printk") 消息可以指定日志级别。
> * 格式字符串虽然与C99基本兼容，但并不遵循完全相同的规范。它有一些扩展和一些限制（没
>   有 `%n` 或浮点转换指定符）。参见:ref: 如何正确地获得printk格式指定符<printk-specifiers> 。

所有的[`printk()`](../../../core-api/printk-basics.html#c.printk "printk")消息都会被打印到内核日志缓冲区，这是一个通过/dev/kmsg输出到用户空间的环
形缓冲区。读取它的通常方法是使用 `dmesg` 。

[`printk()`](../../../core-api/printk-basics.html#c.printk "printk")的用法通常是这样的:

```
printk(KERN_INFO "Message: %s\n", arg);
```

其中 `KERN_INFO` 是日志级别（注意，它与格式字符串连在一起，日志级别不是一个单独的参数）。
可用的日志级别是：

| 名称 | 字符串 | 别名函数 |
| --- | --- | --- |
| KERN\_EMERG | “0” | [`pr_emerg()`](../../../core-api/printk-basics.html#c.pr_emerg "pr_emerg") |
| KERN\_ALERT | “1” | [`pr_alert()`](../../../core-api/printk-basics.html#c.pr_alert "pr_alert") |
| KERN\_CRIT | “2” | [`pr_crit()`](../../../core-api/printk-basics.html#c.pr_crit "pr_crit") |
| KERN\_ERR | “3” | [`pr_err()`](../../../core-api/printk-basics.html#c.pr_err "pr_err") |
| KERN\_WARNING | “4” | [`pr_warn()`](../../../core-api/printk-basics.html#c.pr_warn "pr_warn") |
| KERN\_NOTICE | “5” | [`pr_notice()`](../../../core-api/printk-basics.html#c.pr_notice "pr_notice") |
| KERN\_INFO | “6” | [`pr_info()`](../../../core-api/printk-basics.html#c.pr_info "pr_info") |
| KERN\_DEBUG | “7” | [`pr_debug()`](../../../core-api/printk-basics.html#c.pr_debug "pr_debug") and [`pr_devel()`](../../../core-api/printk-basics.html#c.pr_devel "pr_devel") 若定义了DEBUG |
| KERN\_DEFAULT | “” |  |
| KERN\_CONT | “c” | [`pr_cont()`](../../../core-api/printk-basics.html#c.pr_cont "pr_cont") |

日志级别指定了一条消息的重要性。内核根据日志级别和当前 *console\_loglevel* （一个内核变量）决
定是否立即显示消息（将其打印到当前控制台）。如果消息的优先级比 *console\_loglevel* 高（日志级
别值较低），消息将被打印到控制台。

如果省略了日志级别，则以 `KERN_DEFAULT` 级别打印消息。

你可以用以下方法检查当前的 *console\_loglevel*

```
$ cat /proc/sys/kernel/printk
4        4        1        7
```

结果显示了 *current*, *default*, *minimum* 和 *boot-time-default* 日志级别

要改变当前的 console\_loglevel，只需在 `/proc/sys/kernel/printk` 中写入所需的
级别。例如，要打印所有的消息到控制台上:

```
# echo 8 > /proc/sys/kernel/printk
```

另一种方式，使用 `dmesg`:

```
# dmesg -n 5
```

设置 console\_loglevel 打印 KERN\_WARNING (4) 或更严重的消息到控制台。更多消息参
见 `dmesg(1)` 。

作为[`printk()`](../../../core-api/printk-basics.html#c.printk "printk")的替代方案，你可以使用 `pr_*()` 别名来记录日志。这个系列的宏在宏名中
嵌入了日志级别。例如:

```
pr_info("Info message no. %d\n", msg_num);
```

打印 `KERN_INFO` 消息。

除了比等效的[`printk()`](../../../core-api/printk-basics.html#c.printk "printk")调用更简洁之外，它们还可以通过[`pr_fmt()`](../../../core-api/printk-basics.html#c.pr_fmt "pr_fmt")宏为格式字符串使用一个通用
的定义。例如，在源文件的顶部（在任何 `#include` 指令之前）定义这样的内容。:

```
#define pr_fmt(fmt) "%s:%s: " fmt, KBUILD_MODNAME, __func__
```

会在该文件中的每一条 pr\_\*() 消息前加上发起该消息的模块和函数名称。

为了调试，还有两个有条件编译的宏：
[`pr_debug()`](../../../core-api/printk-basics.html#c.pr_debug "pr_debug")和[`pr_devel()`](../../../core-api/printk-basics.html#c.pr_devel "pr_devel")，除非定义了 `DEBUG` (或者在[`pr_debug()`](../../../core-api/printk-basics.html#c.pr_debug "pr_debug")的情况下定义了
`CONFIG_DYNAMIC_DEBUG` )，否则它们不会被编译。

## 函数接口

该API在以下内核代码中:

include/linux/printk.h
