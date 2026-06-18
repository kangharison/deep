# 架构支持

> 출처(원문): https://docs.kernel.org/translations/zh_CN/rust/arch-support.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Note

此文件的目的是为让中文读者更容易阅读和理解，而不是作为一个分支。 因此，
如果您对此文件有任何意见或更新，请先尝试更新原始英文文件。
如果您发现本文档与原始文件有任何不同或者有翻译问题，请发建议或者补丁给
该文件的译者，或者请求中文文档维护者和审阅者的帮助。

Original:
:   [Arch Support](../../../rust/arch-support.html)

翻译:
:   司延腾 Yanteng Si <[siyanteng@loongson.cn](mailto:siyanteng%40loongson.cn)>

# 架构支持

目前，Rust编译器（`rustc`）使用LLVM进行代码生成，这限制了可以支持的目标架构。此外，对
使用LLVM/Clang构建内核的支持也有所不同（请参见 [Building Linux with Clang/LLVM](../../../kbuild/llvm.html) ）。这
种支持对于使用 `libclang` 的 `bindgen` 来说是必需的。

下面是目前可以工作的架构的一般总结。支持程度与 `MAINTAINERS` 文件中的``S`` 值相对应:

| 架构 | 支持水平 | 限制因素 |
| --- | --- | --- |
| `arm` | Maintained | 仅 ARMv7 小端序。 |
| `arm64` | Maintained | 仅小端序。 |
| `loongarch` | Maintained | - |
| `riscv` | Maintained | 仅 `riscv64`，且仅限 LLVM/Clang。 |
| `um` | Maintained | - |
| `x86` | Maintained | 仅 `x86_64`。 |
