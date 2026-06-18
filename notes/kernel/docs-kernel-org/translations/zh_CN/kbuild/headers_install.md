# 导出内核头文件供用户空间使用

> 출처(원문): https://docs.kernel.org/translations/zh_CN/kbuild/headers_install.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Note

此文件的目的是为让中文读者更容易阅读和理解，而不是作为一个分支。 因此，
如果您对此文件有任何意见或更新，请先尝试更新原始英文文件。
如果您发现本文档与原始文件有任何不同或者有翻译问题，请发建议或者补丁给
该文件的译者，或者请求中文文档维护者和审阅者的帮助。

Original:
:   [Exporting kernel headers for use by userspace](../../../kbuild/headers_install.html)

Translator:
:   慕冬亮 Dongliang Mu <[dzm91@hust.edu.cn](mailto:dzm91%40hust.edu.cn)>

# 导出内核头文件供用户空间使用

“make headers\_install” 命令以适合于用户空间程序的形式导出内核头文件。

Linux 内核导出的头文件描述了用户空间程序尝试使用内核服务的 API。这些内核
头文件被系统的 C 库（例如 glibc 和 uClibc）用于定义可用的系统调用，以及
与这些系统调用一起使用的常量和结构。C 库的头文件包括来自 linux 子目录的
内核头文件。系统的 libc 头文件通常被安装在默认位置 /usr/include，而内核
头文件在该位置的子目录中（主要是 /usr/include/linux 和 /usr/include/asm）。

内核头文件向后兼容，但不向前兼容。这意味着使用旧内核头文件的 C 库构建的程序
可以在新内核上运行（尽管它可能无法访问新特性），但使用新内核头文件构建的程序
可能无法在旧内核上运行。

“make headers\_install” 命令可以在内核源代码的顶层目录中运行（或使用标准
的树外构建）。它接受两个可选参数:

```
make headers_install ARCH=i386 INSTALL_HDR_PATH=/usr
```

ARCH 表明为其生成头文件的架构，默认为当前架构。导出内核头文件的 linux/asm
目录是基于特定平台的，要查看支持架构的完整列表，使用以下命令:

```
ls -d include/asm-* | sed 's/.*-//'
```

INSTALL\_HDR\_PATH 表明头文件的安装位置，默认为 “./usr”。

该命令会在 INSTALL\_HDR\_PATH 中自动创建创建一个 ‘include’ 目录，而头文件
会被安装在 INSTALL\_HDR\_PATH/include 中。

内核头文件导出的基础设施由 David Woodhouse <[dwmw2@infradead.org](mailto:dwmw2%40infradead.org)> 维护。
