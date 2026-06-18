# 2.啓動 Linux/LoongArch

> 출처(원문): https://docs.kernel.org/translations/zh_TW/arch/loongarch/booting.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Warning

此文件的目的是爲讓中文讀者更容易閱讀和理解，而不是作爲一個分支。因此，
如果您對此文件有任何意見或改動，請先嘗試更新原始英文文件。如果要更改或
修正某處翻譯文件，請將意見或補丁發送給維護者（聯繫方式見下）。

Note

如果您發現本文檔與原始文件有任何不同或者有翻譯問題，請聯繫該文件的譯者，
或者發送電子郵件給胡皓文以獲取幫助：<[2023002089@link.tyut.edu.cn](mailto:2023002089%40link.tyut.edu.cn)>。

Original:
:   [Booting Linux/LoongArch](../../../../arch/loongarch/booting.html)

翻譯:
:   司延騰 Yanteng Si <[siyanteng@loongson.cn](mailto:siyanteng%40loongson.cn)>

# 2. 啓動 Linux/LoongArch

作者:
:   司延騰 <[siyanteng@loongson.cn](mailto:siyanteng%40loongson.cn)>

日期:
:   2022年11月18日

## 2.1. BootLoader傳遞給內核的信息

LoongArch支持ACPI和FDT啓動，需要傳遞給內核的信息包括memmap、initrd、cmdline、可
選的ACPI/FDT表等。

內核在 kernel\_entry 入口處被傳遞以下參數:

> * a0 = efi\_boot: efi\_boot 是一個標誌，表示這個啓動環境是否完全符合UEFI
>   的要求。
> * a1 = cmdline: cmdline 是一個指向內核命令行的指針。
> * a2 = systemtable: systemtable 指向EFI的系統表，在這個階段涉及的所有
>   指針都是物理地址。

## 2.2. Linux/LoongArch內核鏡像文件頭

內核鏡像是EFI鏡像。作爲PE文件，它們有一個64字節的頭部結構體，如下所示:

```
u32     MZ_MAGIC                /* "MZ", MS-DOS 頭 */
u32     res0 = 0                /* 保留 */
u64     kernel_entry            /* 內核入口點 */
u64     _end - _text            /* 內核鏡像有效大小 */
u64     load_offset             /* 加載內核鏡像相對內存起始地址的偏移量 */
u64     res1 = 0                /* 保留 */
u64     res2 = 0                /* 保留 */
u64     res3 = 0                /* 保留 */
u32     LINUX_PE_MAGIC          /* 魔術數 */
u32     pe_header - _head       /* 到PE頭的偏移量 */
```
