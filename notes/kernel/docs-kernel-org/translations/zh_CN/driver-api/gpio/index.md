# 通用型输入/输出（GPIO）

> 출처(원문): https://docs.kernel.org/translations/zh_CN/driver-api/gpio/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Note

此文件的目的是为让中文读者更容易阅读和理解，而不是作为一个分支。 因此，
如果您对此文件有任何意见或更新，请先尝试更新原始英文文件。
如果您发现本文档与原始文件有任何不同或者有翻译问题，请发建议或者补丁给
该文件的译者，或者请求中文文档维护者和审阅者的帮助。

Original:
:   [General Purpose Input/Output (GPIO)](../../../../driver-api/gpio/index.html)

翻译:
:   司延腾 Yanteng Si <[siyanteng@loongson.cn](mailto:siyanteng%40loongson.cn)>

校译:

# 通用型输入/输出（GPIO）

Todolist:

* intro
* using-gpio
* driver
* consumer
* board
* drivers-on-gpio
* bt8xxgpio

## 核心

该API在以下内核代码中:

include/linux/gpio/driver.h

drivers/gpio/gpiolib.c

## ACPI支持

该API在以下内核代码中:

drivers/gpio/gpiolib-acpi-core.c

## 设备树支持

该API在以下内核代码中:

drivers/gpio/gpiolib-of.c

## 设备管理支持

该API在以下内核代码中:

drivers/gpio/gpiolib-devres.c

## sysfs帮助（函数）

该API在以下内核代码中:

drivers/gpio/gpiolib-sysfs.c
