# 页表检查

> 출처(원문): https://docs.kernel.org/translations/zh_CN/mm/page_table_check.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 页表检查

## 概述

页表检查允许通过确保防止某些类型的内存损坏来强化内核。

当新的页面可以从用户空间访问时，页表检查通过将它们的页表项（PTEs PMD等）添加到页表中来执行额外
的验证。

在检测到损坏的情况下，内核会被崩溃。页表检查有一个小的性能和内存开销。因此，它在默认情况下是禁用
的，但是在额外的加固超过性能成本的系统上，可以选择启用。另外，由于页表检查是同步的，它可以帮助调
试双映射内存损坏问题，在错误的映射发生时崩溃内核，而不是在内存损坏错误发生后内核崩溃。

## 双重映射检测逻辑

| Current Mapping | New mapping | Permissions | Rule |
| --- | --- | --- | --- |
| Anonymous | Anonymous | Read | Allow |
| Anonymous | Anonymous | Read / Write | Prohibit |
| Anonymous | Named | Any | Prohibit |
| Named | Anonymous | Any | Prohibit |
| Named | Named | Any | Allow |

## 启用页表检查

用以下方法构建内核:

* PAGE\_TABLE\_CHECK=y
  注意，它只能在ARCH\_SUPPORTS\_PAGE\_TABLE\_CHECK可用的平台上启用。
* 使用 “page\_table\_check=on” 内核参数启动。

可以选择用PAGE\_TABLE\_CHECK\_ENFORCED来构建内核，以便在没有额外的内核参数的情况下获得页表
支持。

## 实现注意事项

我们特意决定不使用 VMA 信息，以避免依赖于 MM 状态（除了有限的 “`struct page`” 信息）。页表检查
独立于 Linux-MM 状态机，它验证用户可访问的页面不会被错误地共享。

PAGE\_TABLE\_CHECK 依赖于 EXCLUSIVE\_SYSTEM\_RAM。原因在于，若没有 EXCLUSIVE\_SYSTEM\_RAM，
用户被允许通过 /dev/mem 将任意物理内存区域映射到用户空间。同时，页面可能在映射到用户空间期间
改变自己的属性（例如，从匿名页面变为命名页面），导致页表检查检测到“损坏”。

即使有 EXCLUSIVE\_SYSTEM\_RAM，I/O 页面可能仍然被允许通过 /dev/mem 映射。然而，这些页面始终
被视为命名页面，所以它们不会破坏页表检查中使用的逻辑。
