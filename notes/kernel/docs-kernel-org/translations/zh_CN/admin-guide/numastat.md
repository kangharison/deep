# Numa策略命中/未命中统计

> 출처(원문): https://docs.kernel.org/translations/zh_CN/admin-guide/numastat.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Note

此文件的目的是为让中文读者更容易阅读和理解，而不是作为一个分支。 因此，
如果您对此文件有任何意见或更新，请先尝试更新原始英文文件。
如果您发现本文档与原始文件有任何不同或者有翻译问题，请发建议或者补丁给
该文件的译者，或者请求中文文档维护者和审阅者的帮助。

Original:
:   [Numa policy hit/miss statistics](../../../admin-guide/numastat.html)

Translator:
:   Tao Zou <[wodemia@linux.alibaba.com](mailto:wodemia%40linux.alibaba.com)>

# Numa策略命中/未命中统计

/sys/devices/system/node/node\*/numastat

所有数据的单位都是页面。巨页有独立的计数器。

numa\_hit、numa\_miss和numa\_foreign计数器反映了进程是否能够在他们偏好的节点上分配内存。
如果进程成功在偏好的节点上分配内存则在偏好的节点上增加numa\_hit计数，否则在偏好的节点上增
加numa\_foreign计数同时在实际内存分配的节点上增加numa\_miss计数。

通常，偏好的节点是进程运行所在的CPU的本地节点，但是一些限制可以改变这一行为，比如内存策略，
因此同样有两个基于CPU本地节点的计数器。local\_node和numa\_hit类似，当在CPU所在的节点上分
配内存时增加local\_node计数，other\_node和numa\_miss类似，当在CPU所在节点之外的其他节点
上成功分配内存时增加other\_node计数。需要注意，没有和numa\_foreign对应的计数器。

更多细节内容:

|  |  |
| --- | --- |
| numa\_hit | 一个进程想要从本节点分配内存并且成功。 |
| numa\_miss | 一个进程想要从其他节点分配内存但是最终在本节点完成内存分配。 |
| numa\_foreign | 一个进程想要在本节点分配内存但是最终在其他节点完成内存分配。 |
| local\_node | 一个进程运行在本节点的CPU上并且从本节点上获得了内存。 |
| other\_node | 一个进程运行在其他节点的CPU上但是在本节点上获得了内存。 |
| interleave\_hit | 内存交叉分配策略下想要从本节点分配内存并且成功。 |

你可以使用numactl软件包（<http://oss.sgi.com/projects/libnuma/>）中的numastat工具
来辅助阅读。需要注意，numastat工具目前只在有少量CPU的机器上运行良好。

需要注意，在包含无内存节点（一个节点有CPUs但是没有内存）的系统中numa\_hit、numa\_miss和
numa\_foreign统计数据会被严重曲解。在当前的内核实现中，如果一个进程偏好一个无内存节点（即
进程正在该节点的一个本地CPU上运行），实际上会从距离最近的有内存节点中挑选一个作为偏好节点。
结果会导致相应的内存分配不会增加无内存节点上的numa\_foreign计数器，并且会扭曲最近节点上的
numa\_hit、numa\_miss和numa\_foreign统计数据。
