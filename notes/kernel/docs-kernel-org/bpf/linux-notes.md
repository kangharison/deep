# 1Linux implementation notes

> 출처(원문): https://docs.kernel.org/bpf/linux-notes.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# [1 Linux implementation notes](#id1)

This document provides more details specific to the Linux kernel implementation of the eBPF instruction set.

## [1.1 Byte swap instructions](#id2)

`BPF_FROM_LE` and `BPF_FROM_BE` exist as aliases for `BPF_TO_LE` and `BPF_TO_BE` respectively.

## [1.2 Jump instructions](#id3)

`BPF_CALL | BPF_X | BPF_JMP` (0x8d), where the helper function
integer would be read from a specified register, is not currently supported
by the verifier. Any programs with this instruction will fail to load
until such support is added.

## [1.3 Maps](#id4)

Linux only supports the ‘map\_val(map)’ operation on array maps with a single element.

Linux uses an fd\_array to store maps associated with a BPF program. Thus,
map\_by\_idx(imm) uses the fd at that index in the array.

## [1.4 Variables](#id5)

The following 64-bit immediate instruction specifies that a variable address,
which corresponds to some integer stored in the ‘imm’ field, should be loaded:

| opcode construction | opcode | src | pseudocode | imm type | dst type |
| --- | --- | --- | --- | --- | --- |
| BPF\_IMM | BPF\_DW | BPF\_LD | 0x18 | 0x3 | dst = var\_addr(imm) | variable id | data pointer |

On Linux, this integer is a BTF ID.

## [1.5 Legacy BPF Packet access instructions](#id6)

As mentioned in the [ISA standard documentation](instruction-set.html#legacy-bpf-packet-access-instructions),
Linux has special eBPF instructions for access to packet data that have been
carried over from classic BPF to retain the performance of legacy socket
filters running in the eBPF interpreter.

The instructions come in two forms: `BPF_ABS | <size> | BPF_LD` and
`BPF_IND | <size> | BPF_LD`.

These instructions are used to access packet data and can only be used when
the program context is a pointer to a networking packet. `BPF_ABS`
accesses packet data at an absolute offset specified by the immediate data
and `BPF_IND` access packet data at an offset that includes the value of
a register in addition to the immediate data.

These instructions have seven implicit operands:

* Register R6 is an implicit input that must contain a pointer to a
  [`struct sk_buff`](../networking/kapi.html#c.sk_buff "sk_buff").
* Register R0 is an implicit output which contains the data fetched from
  the packet.
* Registers R1-R5 are scratch registers that are clobbered by the
  instruction.

These instructions have an implicit program exit condition as well. If an
eBPF program attempts access data beyond the packet boundary, the
program execution will be aborted.

`BPF_ABS | BPF_W | BPF_LD` (0x20) means:

```
R0 = ntohl(*(u32 *) ((struct sk_buff *) R6->data + imm))
```

where `ntohl()` converts a 32-bit value from network byte order to host byte order.

`BPF_IND | BPF_W | BPF_LD` (0x40) means:

```
R0 = ntohl(*(u32 *) ((struct sk_buff *) R6->data + src + imm))
```
