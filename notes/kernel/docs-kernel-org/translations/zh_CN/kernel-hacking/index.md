# 内核骇客指南

> 출처(원문): https://docs.kernel.org/translations/zh_CN/kernel-hacking/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Note

此文件的目的是为让中文读者更容易阅读和理解，而不是作为一个分支。 因此，
如果您对此文件有任何意见或更新，请先尝试更新原始英文文件。
如果您发现本文档与原始文件有任何不同或者有翻译问题，请发建议或者补丁给
该文件的译者，或者请求中文文档维护者和审阅者的帮助。

Original:
:   [Kernel Hacking Guides](../../../kernel-hacking/index.html)

译者:
:   吴想成 Wu XiangCheng <[bobwxc@email.cn](mailto:bobwxc%40email.cn)>

# 内核骇客指南

* [内核骇客指北](hacking.html)
  + [引言](hacking.html#id2)
  + [玩家](hacking.html#id3)
  + [一些基本规则](hacking.html#id5)
  + [输入输出控制（ioctls）：避免编写新的系统调用](hacking.html#ioctls)
  + [死锁的“配方”](hacking.html#id6)
  + [常用函数/程序](hacking.html#id7)
  + [等待队列 `include/linux/wait.h`](hacking.html#include-linux-wait-h)
  + [原子操作](hacking.html#id11)
  + [符号](hacking.html#id12)
  + [程序与惯例](hacking.html#id13)
  + [把你的东西放进内核里](hacking.html#id17)
  + [Kernel 仙女棒](hacking.html#kernel)
  + [致谢](hacking.html#id18)

TODO

* locking
