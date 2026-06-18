# 1BPF ABI Recommended Conventions and Guidelines v1.0

> 출처(원문): https://docs.kernel.org/bpf/standardization/abi.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# [1 BPF ABI Recommended Conventions and Guidelines v1.0](#id1)

This is version 1.0 of an informational document containing recommended
conventions and guidelines for producing portable BPF program binaries.

## [1.1 Registers and calling convention](#id2)

BPF has 10 general purpose registers and a read-only frame pointer register,
all of which are 64-bits wide.

The BPF calling convention is defined as:

* R0: return value from function calls, and exit value for BPF programs
* R1 - R5: arguments for function calls
* R6 - R9: callee saved registers that function calls will preserve
* R10: read-only frame pointer to access stack

R0 - R5 are scratch registers and BPF programs needs to spill/fill them if
necessary across calls.

The BPF program needs to store the return value into register R0 before doing an
`EXIT`.
