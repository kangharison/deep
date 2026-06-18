# 思睿逻辑 EP93xx 模拟数字转换器驱动

> 출처(원문): https://docs.kernel.org/translations/zh_CN/iio/ep93xx_adc.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Note

此文件的目的是为让中文读者更容易阅读和理解，而不是作为一个分支。 因此，
如果您对此文件有任何意见或更新，请先尝试更新原始英文文件。
如果您发现本文档与原始文件有任何不同或者有翻译问题，请发建议或者补丁给
该文件的译者，或者请求中文文档维护者和审阅者的帮助。

Original:
:   [Cirrus Logic EP93xx ADC driver](../../../iio/ep93xx_adc.html)

翻译:
:   司延腾 Yanteng Si <[siyanteng@loongson.cn](mailto:siyanteng%40loongson.cn)>

# 思睿逻辑 EP93xx 模拟数字转换器驱动

## 1. 概述

该驱动同时适用于具有5通道模拟数字转换器的低端 (EP9301, Ep9302) 设备和10通道
触摸屏/模拟数字转换器的高端设备(EP9307, EP9312, EP9315)。

## 2. 通道编号

EP9301和EP9302数据表定义了通道0..4的编号方案。虽然EP9307, EP9312和EP9315多
了3个通道（一共8个），但是编号并没有定义。所以说最后三个通道是随机编号的。

如果ep93xx\_adc是IIO设备0，您将在以下位置找到条目
/sys/bus/iio/devices/iio:device0/:

> | sysfs 入口 | ball/pin 名称 |
> | --- | --- |
> | in\_voltage0\_raw | YM |
> | in\_voltage1\_raw | SXP |
> | in\_voltage2\_raw | SXM |
> | in\_voltage3\_raw | SYP |
> | in\_voltage4\_raw | SYM |
> | in\_voltage5\_raw | XP |
> | in\_voltage6\_raw | XM |
> | in\_voltage7\_raw | YP |
