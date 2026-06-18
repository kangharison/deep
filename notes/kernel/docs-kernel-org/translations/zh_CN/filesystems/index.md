# Linux Kernel中的文件系统

> 출처(원문): https://docs.kernel.org/translations/zh_CN/filesystems/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Note

此文件的目的是为让中文读者更容易阅读和理解，而不是作为一个分支。 因此，
如果您对此文件有任何意见或更新，请先尝试更新原始英文文件。
如果您发现本文档与原始文件有任何不同或者有翻译问题，请发建议或者补丁给
该文件的译者，或者请求中文文档维护者和审阅者的帮助。

Original:
:   [Documentation/filesystems/index.rst](../../../filesystems/index.html#filesystems-index)

Translator:
:   Wang Wenhu <[wenhu.wang@vivo.com](mailto:wenhu.wang%40vivo.com)>

# Linux Kernel中的文件系统

这份正在开发的手册或许在未来某个辉煌的日子里以易懂的形式将Linux虚拟文件系统（VFS）层以及基于其上的各种文件系统如何工作呈现给大家。当前可以看到下面的内容。

## 核心 VFS 文档

有关 VFS 层本身以及其算法工作方式的文档，请参阅这些手册。

* [Linux 目录通知](dnotify.html)

## 文件系统

文件系统实现文档。

* [virtiofs: virtio-fs 主机<->客机共享文件系统](virtiofs.html)
  + [介绍](virtiofs.html#id1)
  + [用法](virtiofs.html#id2)
  + [内幕](virtiofs.html#id3)
* [Debugfs](debugfs.html)
* [Tmpfs](tmpfs.html)
* [UBI 文件系统](ubifs.html)
  + [简介](ubifs.html#id1)
  + [挂载选项](ubifs.html#id2)
  + [快速使用指南](ubifs.html#id3)
  + [参考资料](ubifs.html#id4)
* [UBIFS认证支持](ubifs-authentication.html)
  + [引言](ubifs-authentication.html#id1)
  + [UBIFS认证](ubifs-authentication.html#id6)
  + [未来扩展](ubifs-authentication.html#id12)
  + [参考](ubifs-authentication.html#id13)
* [全局文件系统 2 (Global File System 2)](gfs2.html)
* [uevents 与 GFS2](gfs2-uevents.html)
  + [GFS2 uevents 列表](gfs2-uevents.html#gfs2-uevents)
  + [所有 GFS2 uevents 的通用信息（uevent 环境变量）](gfs2-uevents.html#gfs2-uevents-uevent)
* [Glock 内部加锁规则](gfs2-glocks.html)
  + [Glock 统计](gfs2-glocks.html#id1)
* [Inotify - 一个强大且简单的文件变更通知系统](inotify.html)
