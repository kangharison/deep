# ACRN CPUID位域

> 출처(원문): https://docs.kernel.org/translations/zh_CN/virt/acrn/cpuid.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Note

此文件的目的是为让中文读者更容易阅读和理解，而不是作为一个分支。 因此，
如果您对此文件有任何意见或更新，请先尝试更新原始英文文件。
如果您发现本文档与原始文件有任何不同或者有翻译问题，请发建议或者补丁给
该文件的译者，或者请求中文文档维护者和审阅者的帮助。

Original:
:   [ACRN CPUID bits](../../../../virt/acrn/cpuid.html)

翻译:
:   司延腾 Yanteng Si <[siyanteng@loongson.cn](mailto:siyanteng%40loongson.cn)>

校译:
:   时奎亮 Alex Shi <[alexs@kernel.org](mailto:alexs%40kernel.org)>

# ACRN CPUID位域

在ACRN超级管理器上运行的客户虚拟机可以使用CPUID检查其一些功能。

ACRN的cpuid函数是:

函数: 0x40000000

返回:

```
eax = 0x40000010
ebx = 0x4e524341
ecx = 0x4e524341
edx = 0x4e524341
```

注意，ebx，ecx和edx中的这个值对应于字符串“ACRNACRNACRN”。eax中的值对应于这个叶子
中存在的最大cpuid函数，如果将来有更多的函数加入，将被更新。

函数: define ACRN\_CPUID\_FEATURES (0x40000001)

返回:

```
ebx, ecx, edx
eax = an OR'ed group of (1 << flag)
```

其中 `flag` 的定义如下:

| 标志 | 值 | 描述 |
| --- | --- | --- |
| ACRN\_FEATURE\_PRIVILEGED\_VM | 0 | 客户虚拟机是一个有特权的虚拟机 |

函数: 0x40000010

返回:

```
ebx, ecx, edx
eax = (Virtual) TSC frequency in kHz.
```
