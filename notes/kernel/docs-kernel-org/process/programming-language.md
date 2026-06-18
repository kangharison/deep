# Programming Language

> 출처(원문): https://docs.kernel.org/process/programming-language.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Programming Language

The Linux kernel is written in the C programming language [[c-language]](#c-language).
More precisely, it is typically compiled with `gcc` [[gcc]](#gcc)
under `-std=gnu11` [[gcc-c-dialect-options]](#gcc-c-dialect-options): the GNU dialect of ISO C11.
`clang` [[clang]](#clang) is also supported; see documentation on
[Building Linux with Clang/LLVM](../kbuild/llvm.html#kbuild-llvm).

This dialect contains many extensions to the language [[gnu-extensions]](#gnu-extensions),
and many of them are used within the kernel as a matter of course.

## Attributes

One of the common extensions used throughout the kernel are attributes
[[gcc-attribute-syntax]](#gcc-attribute-syntax). Attributes allow to introduce
implementation-defined semantics to language entities (like variables,
functions or types) without having to make significant syntactic changes
to the language (e.g. adding a new keyword) [[n2049]](#n2049).

In some cases, attributes are optional (i.e. a compiler not supporting them
should still produce proper code, even if it is slower or does not perform
as many compile-time checks/diagnostics).

The kernel defines pseudo-keywords (e.g. `__pure`) instead of using
directly the GNU attribute syntax (e.g. `__attribute__((__pure__))`)
in order to feature detect which ones can be used and/or to shorten the code.

Please refer to `include/linux/compiler_attributes.h` for more information.

## Rust

The kernel has support for the Rust programming language
[[rust-language]](#rust-language) under `CONFIG_RUST`. It is compiled with `rustc` [[rustc]](#rustc)
under `--edition=2021` [[rust-editions]](#rust-editions). Editions are a way to introduce
small changes to the language that are not backwards compatible.

On top of that, some unstable features [[rust-unstable-features]](#rust-unstable-features) are used in
the kernel. Unstable features may change in the future, thus it is an important
goal to reach a point where only stable features are used.

Please refer to [Rust](../rust/index.html) for more information.

[[c-language](#id2)]

<http://www.open-std.org/jtc1/sc22/wg14/www/standards>

[[gcc](#id3)]

<https://gcc.gnu.org>

[[clang](#id5)]

<https://clang.llvm.org>

[[gcc-c-dialect-options](#id4)]

<https://gcc.gnu.org/onlinedocs/gcc/C-Dialect-Options.html>

[[gnu-extensions](#id6)]

<https://gcc.gnu.org/onlinedocs/gcc/C-Extensions.html>

[[gcc-attribute-syntax](#id7)]

<https://gcc.gnu.org/onlinedocs/gcc/Attribute-Syntax.html>

[[n2049](#id8)]

<http://www.open-std.org/jtc1/sc22/wg14/www/docs/n2049.pdf>

[[rust-language](#id9)]

<https://www.rust-lang.org>

[[rustc](#id10)]

<https://doc.rust-lang.org/rustc/>

[[rust-editions](#id11)]

<https://doc.rust-lang.org/edition-guide/editions/>

[[rust-unstable-features](#id12)]

<https://github.com/Rust-for-Linux/linux/issues/2>
