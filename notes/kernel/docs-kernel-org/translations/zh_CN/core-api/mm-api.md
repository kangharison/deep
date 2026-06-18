# 内存管理APIs

> 출처(원문): https://docs.kernel.org/translations/zh_CN/core-api/mm-api.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Note

此文件的目的是为让中文读者更容易阅读和理解，而不是作为一个分支。 因此，
如果您对此文件有任何意见或更新，请先尝试更新原始英文文件。
如果您发现本文档与原始文件有任何不同或者有翻译问题，请发建议或者补丁给
该文件的译者，或者请求中文文档维护者和审阅者的帮助。

Original:
:   [Memory Management APIs](../../../core-api/mm-api.html)

翻译:
:   司延腾 Yanteng Si <[siyanteng@loongson.cn](mailto:siyanteng%40loongson.cn)>
    周彬彬 Binbin Zhou <[zhoubinbin@loongson.cn](mailto:zhoubinbin%40loongson.cn)>

校译:
:   时奎亮<[alexs@kernel.org](mailto:alexs%40kernel.org)>

# 内存管理APIs

API（Application Programming Interface，应用程序接口）

## 用户空间内存访问

该API在以下内核代码中:

arch/x86/include/asm/uaccess.h

arch/x86/lib/usercopy\_32.c

mm/gup.c

## 内存分配控制

该API在以下内核代码中:

include/linux/gfp\_types.h

## Slab缓存

此缓存非cpu片上缓存，请读者自行查阅资料。

该API在以下内核代码中:

include/linux/slab.h

mm/slab.c

mm/slab\_common.c

mm/util.c

## 虚拟连续（内存页）映射

该API在以下内核代码中:

mm/vmalloc.c

## 文件映射和页面缓存

该API在以下内核代码中:

### 文件映射

mm/filemap.c

### 预读

mm/readahead.c

### 回写

mm/page-writeback.c

### 截断

mm/truncate.c

include/linux/pagemap.h

## 内存池

该API在以下内核代码中:

mm/mempool.c

## DMA池

DMA(Direct Memory Access，直接存储器访问)

该API在以下内核代码中:

mm/dmapool.c

## 更多的内存管理函数

该API在以下内核代码中:

mm/memory.c

mm/page\_alloc.c

mm/mempolicy.c

include/linux/mm\_types.h

include/linux/mm\_inline.h

include/linux/page-flags.h

include/linux/mm.h

include/linux/page\_ref.h

include/linux/mmzone.h

mm/util.c
