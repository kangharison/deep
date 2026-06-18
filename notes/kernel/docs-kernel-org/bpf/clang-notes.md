# 1Clang implementation notes

> 출처(원문): https://docs.kernel.org/bpf/clang-notes.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# [1 Clang implementation notes](#id1)

This document provides more details specific to the Clang/LLVM implementation of the eBPF instruction set.

## [1.1 Versions](#id2)

Clang defined “CPU” versions, where a CPU version of 3 corresponds to the current eBPF ISA.

Clang can select the eBPF ISA version using `-mcpu=v3` for example to select version 3.

## [1.2 Arithmetic instructions](#id3)

For CPU versions prior to 3, Clang v7.0 and later can enable `BPF_ALU` support with
`-Xclang -target-feature -Xclang +alu32`. In CPU version 3, support is automatically included.

## [1.3 Jump instructions](#id4)

If `-O0` is used, Clang will generate the `BPF_CALL | BPF_X | BPF_JMP` (0x8d)
instruction, which is not supported by the Linux kernel verifier.

## [1.4 Atomic operations](#id5)

Clang can generate atomic instructions by default when `-mcpu=v3` is
enabled. If a lower version for `-mcpu` is set, the only atomic instruction
Clang can generate is `BPF_ADD` *without* `BPF_FETCH`. If you need to enable
the atomics features, while keeping a lower `-mcpu` version, you can use
`-Xclang -target-feature -Xclang +alu32`.
