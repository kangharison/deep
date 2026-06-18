# 全局文件系统 2 (Global File System 2)

> 출처(원문): https://docs.kernel.org/translations/zh_CN/filesystems/gfs2.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Note

此文件的目的是为让中文读者更容易阅读和理解，而不是作为一个分支。 因此，
如果您对此文件有任何意见或更新，请先尝试更新原始英文文件。
如果您发现本文档与原始文件有任何不同或者有翻译问题，请发建议或者补丁给
该文件的译者，或者请求中文文档维护者和审阅者的帮助。

Original:
:   Documentation/filesystems/gfs2.rst

翻译:
:   邵明寅 Shao Mingyin <[shao.mingyin@zte.com.cn](mailto:shao.mingyin%40zte.com.cn)>

校译:
:   杨涛 yang tao <[yang.tao172@zte.com.cn](mailto:yang.tao172%40zte.com.cn)>

# 全局文件系统 2 (Global File System 2)

GFS2 是一个集群文件系统。它允许一组计算机同时使用在它们之间共享的块设备（通
过 FC、iSCSI、NBD 等）。GFS2 像本地文件系统一样读写块设备，但也使用一个锁
模块来让计算机协调它们的 I/O 操作，从而维护文件系统的一致性。GFS2 的出色特
性之一是完美一致性——在一台机器上对文件系统所做的更改会立即显示在集群中的所
有其他机器上。

GFS2 使用可互换的节点间锁定机制，当前支持的机制有：

> lock\_nolock
> :   * 允许将 GFS2 用作本地文件系统
>
> lock\_dlm
> :   * 使用分布式锁管理器 (dlm) 进行节点间锁定。
>       该 dlm 位于 linux/fs/dlm/

lock\_dlm 依赖于在上述 URL 中找到的用户空间集群管理系统。

若要将 GFS2 用作本地文件系统，则不需要外部集群系统，只需：:

> $ mkfs -t gfs2 -p lock\_nolock -j 1 /dev/block\_device
> $ mount -t gfs2 /dev/block\_device /dir

在所有集群节点上都需要安装 gfs2-utils 软件包；对于 lock\_dlm，您还需要按
照文档配置 dlm 和 corosync 用户空间工具。

gfs2-utils 可在 <https://pagure.io/gfs2-utils> 找到。

GFS2 在磁盘格式上与早期版本的 GFS 不兼容，但它已相当接近。

以下手册页 (man pages) 可在 gfs2-utils 中找到：

> |  |  |
> | --- | --- |
> | fsck.gfs2 | 用于修复文件系统 |
> | gfs2\_grow | 用于在线扩展文件系统 |
> | gfs2\_jadd | 用于在线向文件系统添加日志 |
> | tunegfs2 | 用于操作、检查和调优文件系统 |
> | gfs2\_convert | 用于将 gfs 文件系统原地转换为 GFS2 |
> | mkfs.gfs2 | 用于创建文件系统 |
