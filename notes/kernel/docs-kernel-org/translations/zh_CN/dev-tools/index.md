# 内核开发工具

> 출처(원문): https://docs.kernel.org/translations/zh_CN/dev-tools/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Note

此文件的目的是为让中文读者更容易阅读和理解，而不是作为一个分支。 因此，
如果您对此文件有任何意见或更新，请先尝试更新原始英文文件。
如果您发现本文档与原始文件有任何不同或者有翻译问题，请发建议或者补丁给
该文件的译者，或者请求中文文档维护者和审阅者的帮助。

Original:
:   [Development tools for the kernel](../../../dev-tools/index.html)

Translator:
:   赵军奎 Bernard Zhao <[bernard@vivo.com](mailto:bernard%40vivo.com)>

# 内核开发工具

本文档是有关内核开发工具文档的合集。
目前这些文档已经整理在一起，不需要再花费额外的精力。
欢迎任何补丁。

有关测试专用工具的简要概述，参见
[内核测试指南](testing-overview.html)

目录

* [内核测试指南](testing-overview.html)
  + [编写和运行测试](testing-overview.html#id2)
  + [代码覆盖率工具](testing-overview.html#id3)
  + [动态分析工具](testing-overview.html#id4)
  + [静态分析工具](testing-overview.html#id5)
* [Sparse](sparse.html)
  + [使用 sparse 工具做类型检查](sparse.html#id1)
  + [获取 sparse 工具](sparse.html#id3)
  + [使用 sparse 工具](sparse.html#id4)
* [KCOV: 用于模糊测试的代码覆盖率](kcov.html)
  + [先决条件](kcov.html#id1)
  + [覆盖率收集](kcov.html#id2)
  + [操作数比较收集](kcov.html#id3)
  + [远程覆盖率收集](kcov.html#id4)
* [内核并发消毒剂(KCSAN)](kcsan.html)
  + [使用](kcsan.html#id1)
  + [数据竞争](kcsan.html#id5)
  + [数据竞争以外的竞争检测](kcsan.html#id7)
  + [实现细节](kcsan.html#id8)
  + [考虑的替代方案](kcsan.html#id11)
* [内核内存消毒剂（KMSAN）](kmsan.html)
  + [使用方法](kmsan.html#id1)
  + [支持](kmsan.html#id5)
  + [KMSAN 的工作原理](kmsan.html#id6)
  + [参考文献](kmsan.html#id21)
* [在Linux内核里使用gcov做代码覆盖率检查](gcov.html)
  + [准备](gcov.html#id1)
  + [定制化](gcov.html#id2)
  + [相关文件](gcov.html#id3)
  + [针对模块的统计](gcov.html#id4)
  + [编译机和测试机分离](gcov.html#id5)
  + [关于编译器的注意事项](gcov.html#id6)
  + [问题定位](gcov.html#id8)
  + [附录A：collect\_on\_build.sh](gcov.html#a-collect-on-build-sh)
  + [附录B：collect\_on\_test.sh](gcov.html#b-collect-on-test-sh)
* [内核地址消毒剂(KASAN)](kasan.html)
  + [概述](kasan.html#id1)
  + [支持](kasan.html#id2)
  + [用法](kasan.html#id6)
  + [实施细则](kasan.html#id9)
  + [影子内存](kasan.html#id13)
  + [对于开发者](kasan.html#id15)
* [未定义行为消毒剂 - UBSAN](ubsan.html)
  + [报告样例](ubsan.html#id1)
  + [用法](ubsan.html#id2)
  + [参考文献](ubsan.html#id3)
* [内核内存泄露检测器](kmemleak.html)
  + [用法](kmemleak.html#id3)
  + [基础算法](kmemleak.html#id4)
  + [用 kmemleak 测试特定部分](kmemleak.html#kmemleak)
  + [释放 kmemleak 内核对象](kmemleak.html#id5)
  + [Kmemleak API](kmemleak.html#kmemleak-api)
  + [解决假阳性/假阴性](kmemleak.html#id6)
  + [限制和缺点](kmemleak.html#id7)
  + [使用 kmemleak-test 测试](kmemleak.html#kmemleak-test)
* [通过gdb调试内核和模块](gdb-kernel-debugging.html)
  + [环境配置要求](gdb-kernel-debugging.html#id1)
  + [设置](gdb-kernel-debugging.html#id2)
  + [使用Linux提供的gdb脚本的示例](gdb-kernel-debugging.html#linuxgdb)
  + [命令和辅助调试功能列表](gdb-kernel-debugging.html#id3)

Todolist:

> * checkpatch
> * coccinelle
> * kfence
> * kgdb
> * kselftest
> * kunit/index
> * ktap
> * checkuapi
