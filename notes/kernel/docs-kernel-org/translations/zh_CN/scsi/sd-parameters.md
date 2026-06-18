# Linux SCSI磁盘驱动（sd）参数

> 출처(원문): https://docs.kernel.org/translations/zh_CN/scsi/sd-parameters.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Note

此文件的目的是为让中文读者更容易阅读和理解，而不是作为一个分支。 因此，
如果您对此文件有任何意见或更新，请先尝试更新原始英文文件。
如果您发现本文档与原始文件有任何不同或者有翻译问题，请发建议或者补丁给
该文件的译者，或者请求中文文档维护者和审阅者的帮助。

Original:
:   [Linux SCSI Disk Driver (sd) Parameters](../../../scsi/sd-parameters.html)

翻译:
:   郝栋栋 doubled <[doubled@leap-io-kernel.com](mailto:doubled%40leap-io-kernel.com)>

校译:

# Linux SCSI磁盘驱动（sd）参数

## 缓存类型（读/写）

启用/禁用驱动器读写缓存。

| 缓存类型字符串 | WCE | RCD | 写缓存 | 读缓存 |
| --- | --- | --- | --- | --- |
| write through | 0 | 0 | 关闭 | 开启 |
| none | 0 | 1 | 关闭 | 关闭 |
| write back | 1 | 0 | 开启 | 开启 |
| write back, no read (daft) | 1 | 1 | 开启 | 关闭 |

将缓存类型设置为“write back”并将该设置保存到驱动器:

```
# echo "write back" > cache_type
```

如果要修改缓存模式但不使更改持久化，可在缓存类型字符串前
添加“temporary ”。例如:

```
# echo "temporary write back" > cache_type
```
