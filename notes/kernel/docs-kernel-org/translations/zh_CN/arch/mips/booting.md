# 1.BMIPS设备树引导

> 출처(원문): https://docs.kernel.org/translations/zh_CN/arch/mips/booting.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Note

此文件的目的是为让中文读者更容易阅读和理解，而不是作为一个分支。 因此，
如果您对此文件有任何意见或更新，请先尝试更新原始英文文件。
如果您发现本文档与原始文件有任何不同或者有翻译问题，请发建议或者补丁给
该文件的译者，或者请求中文文档维护者和审阅者的帮助。

Original:
:   [BMIPS DeviceTree Booting](../../../../arch/mips/booting.html)

翻译:
:   司延腾 Yanteng Si <[siyanteng@loongson.cn](mailto:siyanteng%40loongson.cn)>

# 1. BMIPS设备树引导

> 一些bootloaders只支持在内核镜像开始地址处的单一入口点。而其它
> bootloaders将跳转到ELF的开始地址处。两种方案都支持的；因为
> CONFIG\_BOOT\_RAW=y and CONFIG\_NO\_EXCEPT\_FILL=y, 所以第一条指令
> 会立即跳转到`kernel_entry()`入口处执行。
>
> 与arch/arm情况(b)类似，dt感知的引导加载程序需要设置以下寄存器:
>
> > a0 : 0
> >
> > a1 : 0xffffffff
> >
> > a2RAM中指向设备树块的物理指针(在chapterII中定义)。
> > :   设备树可以位于前512MB物理地址空间(0x00000000 -
> >     0x1fffffff)的任何位置，以64位边界对齐。
>
> 传统bootloaders不会使用这样的约定，并且它们不传入DT块。
> 在这种情况下，Linux将通过选中CONFIG\_DT\_\*查找DTB。
>
> 以上约定只在32位系统中定义，因为目前没有任何64位的BMIPS实现。
