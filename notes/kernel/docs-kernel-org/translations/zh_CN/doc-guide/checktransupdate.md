# 工作原理

> 출처(원문): https://docs.kernel.org/translations/zh_CN/doc-guide/checktransupdate.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Note

此文件的目的是为让中文读者更容易阅读和理解，而不是作为一个分支。 因此，
如果您对此文件有任何意见或更新，请先尝试更新原始英文文件。
如果您发现本文档与原始文件有任何不同或者有翻译问题，请发建议或者补丁给
该文件的译者，或者请求中文文档维护者和审阅者的帮助。

Original:
:   [Checking for needed translation updates](../../../doc-guide/checktransupdate.html)

译者:
:   慕冬亮 Dongliang Mu <[dzm91@hust.edu.cn](mailto:dzm91%40hust.edu.cn)>

检查翻译更新

这个脚本帮助跟踪不同语言的文档翻译状态，即文档是否与对应的英文版本保持更新。

# 工作原理

它使用 `git log` 命令来跟踪翻译提交的最新英文提交（按作者日期排序）和英文文档的
最新提交。如果有任何差异，则该文件被认为是过期的，然后需要更新的提交将被收集并报告。

实现的功能

* 检查特定语言中的所有文件
* 检查单个文件或一组文件
* 提供更改输出格式的选项
* 跟踪没有翻译过的文件的翻译状态

# 用法

```
tools/docs/checktransupdate.py --help
```

具体用法请参考参数解析器的输出

示例

* `tools/docs/checktransupdate.py -l zh_CN`
  这将打印 zh\_CN 语言中需要更新的所有文件。
* `tools/docs/checktransupdate.py Documentation/translations/zh_CN/dev-tools/testing-overview.rst`
  这将只打印指定文件的状态。

然后输出类似如下的内容：

```
Documentation/dev-tools/kfence.rst
No translation in the locale of zh_CN

Documentation/translations/zh_CN/dev-tools/testing-overview.rst
commit 42fb9cfd5b18 ("Documentation: dev-tools: Add link to RV docs")
1 commits needs resolving in total
```

待实现的功能

* 文件参数可以是文件夹而不仅仅是单个文件
