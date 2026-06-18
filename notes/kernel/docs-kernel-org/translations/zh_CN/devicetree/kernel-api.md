# 内核中的设备树API

> 출처(원문): https://docs.kernel.org/translations/zh_CN/devicetree/kernel-api.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Note

此文件的目的是为让中文读者更容易阅读和理解，而不是作为一个分支。 因此，
如果您对此文件有任何意见或更新，请先尝试更新原始英文文件。
如果您发现本文档与原始文件有任何不同或者有翻译问题，请发建议或者补丁给
该文件的译者，或者请求中文文档维护者和审阅者的帮助。

Original:
:   [DeviceTree Kernel API](../../../devicetree/kernel-api.html)

翻译:
:   司延腾 Yanteng Si <[siyanteng@loongson.cn](mailto:siyanteng%40loongson.cn)>

校译:

# 内核中的设备树API

## 核心函数

该API在以下内核代码中:

drivers/of/base.c

include/linux/of.h

drivers/of/property.c

include/linux/of\_graph.h

drivers/of/address.c

drivers/of/irq.c

drivers/of/fdt.c

## 驱动模型函数

该API在以下内核代码中:

include/linux/of\_device.h

drivers/of/device.c

include/linux/of\_platform.h

drivers/of/platform.c

## 覆盖和动态DT函数

该API在以下内核代码中:

drivers/of/resolver.c

drivers/of/dynamic.c

drivers/of/overlay.c
