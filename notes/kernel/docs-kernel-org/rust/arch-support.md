# Arch Support

> 출처(원문): https://docs.kernel.org/rust/arch-support.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Arch Support

Currently, the Rust compiler (`rustc`) uses LLVM for code generation,
which limits the supported architectures that can be targeted. In addition,
support for building the kernel with LLVM/Clang varies (please see
[Building Linux with Clang/LLVM](../kbuild/llvm.html)). This support is needed for `bindgen`
which uses `libclang`.

Below is a general summary of architectures that currently work. Level of
support corresponds to `S` values in the `MAINTAINERS` file.

| Architecture | Level of support | Constraints |
| --- | --- | --- |
| `arm` | Maintained | ARMv7 Little Endian only. |
| `arm64` | Maintained | Little Endian only. |
| `loongarch` | Maintained | - |
| `riscv` | Maintained | `riscv64` and LLVM/Clang only. |
| `um` | Maintained | - |
| `x86` | Maintained | `x86_64` only. |
