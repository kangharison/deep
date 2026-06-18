# POWERPC ELF HWCAPs

> 출처(원문): https://docs.kernel.org/arch/powerpc/elf_hwcaps.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# POWERPC ELF HWCAPs

This document describes the usage and semantics of the powerpc ELF HWCAPs.

## 1. Introduction

Some hardware or software features are only available on some CPU
implementations, and/or with certain kernel configurations, but have no other
discovery mechanism available to userspace code. The kernel exposes the
presence of these features to userspace through a set of flags called HWCAPs,
exposed in the auxiliary vector.

Userspace software can test for features by acquiring the AT\_HWCAP or
AT\_HWCAP2 entry of the auxiliary vector, and testing whether the relevant
flags are set, e.g.:

```
bool floating_point_is_present(void)
{
        unsigned long HWCAPs = getauxval(AT_HWCAP);
        if (HWCAPs & PPC_FEATURE_HAS_FPU)
                return true;

        return false;
}
```

Where software relies on a feature described by a HWCAP, it should check the
relevant HWCAP flag to verify that the feature is present before attempting to
make use of the feature.

HWCAP is the preferred method to test for the presence of a feature rather
than probing through other means, which may not be reliable or may cause
unpredictable behaviour.

Software that targets a particular platform does not necessarily have to
test for required or implied features. For example if the program requires
FPU, VMX, VSX, it is not necessary to test those HWCAPs, and it may be
impossible to do so if the compiler generates code requiring those features.

## 2. Facilities

The Power ISA uses the term “facility” to describe a class of instructions,
registers, interrupts, etc. The presence or absence of a facility indicates
whether this class is available to be used, but the specifics depend on the
ISA version. For example, if the VSX facility is available, the VSX
instructions that can be used differ between the v3.0B and v3.1B ISA
versions.

## 3. Categories

The Power ISA before v3.0 uses the term “category” to describe certain
classes of instructions and operating modes which may be optional or
mutually exclusive, the exact meaning of the HWCAP flag may depend on
context, e.g., the presence of the BOOKE feature implies that the server
category is not implemented.

## 4. HWCAP allocation

HWCAPs are allocated as described in Power Architecture 64-Bit ELF V2 ABI
Specification (which will be reflected in the kernel’s uapi headers).

## 5. The HWCAPs exposed in AT\_HWCAP

PPC\_FEATURE\_32
:   32-bit CPU

PPC\_FEATURE\_64
:   64-bit CPU (userspace may be running in 32-bit mode).

PPC\_FEATURE\_601\_INSTR
:   The processor is PowerPC 601.
    Unused in the kernel since f0ed73f3fa2c (“powerpc: Remove PowerPC 601”)

PPC\_FEATURE\_HAS\_ALTIVEC
:   Vector (aka Altivec, VMX) facility is available.

PPC\_FEATURE\_HAS\_FPU
:   Floating point facility is available.

PPC\_FEATURE\_HAS\_MMU
:   Memory management unit is present and enabled.

PPC\_FEATURE\_HAS\_4xxMAC
:   The processor is 40x or 44x family.
    Unused in the kernel since 732b32daef80 (“powerpc: Remove core support for 40x”)

PPC\_FEATURE\_UNIFIED\_CACHE
:   The processor has a unified L1 cache for instructions and data, as
    found in NXP e200.
    Unused in the kernel since 39c8bf2b3cc1 (“powerpc: Retire e200 core (mpc555x processor)”)

PPC\_FEATURE\_HAS\_SPE
:   Signal Processing Engine facility is available.

PPC\_FEATURE\_HAS\_EFP\_SINGLE
:   Embedded Floating Point single precision operations are available.

PPC\_FEATURE\_HAS\_EFP\_DOUBLE
:   Embedded Floating Point double precision operations are available.

PPC\_FEATURE\_NO\_TB
:   The timebase facility (mftb instruction) is not available.
    This is a 601 specific HWCAP, so if it is known that the processor
    running is not a 601, via other HWCAPs or other means, it is not
    required to test this bit before using the timebase.
    Unused in the kernel since f0ed73f3fa2c (“powerpc: Remove PowerPC 601”)

PPC\_FEATURE\_POWER4
:   The processor is POWER4 or PPC970/FX/MP.
    POWER4 support dropped from the kernel since 471d7ff8b51b (“powerpc/64s: Remove POWER4 support”)

PPC\_FEATURE\_POWER5
:   The processor is POWER5.

PPC\_FEATURE\_POWER5\_PLUS
:   The processor is POWER5+.

PPC\_FEATURE\_CELL
:   The processor is Cell.

PPC\_FEATURE\_BOOKE
:   The processor implements the embedded category (“BookE”) architecture.

PPC\_FEATURE\_SMT
:   The processor implements SMT.

PPC\_FEATURE\_ICACHE\_SNOOP
:   The processor icache is coherent with the dcache, and instruction storage
    can be made consistent with data storage for the purpose of executing
    instructions with the sequence (as described in, e.g., POWER9 Processor
    User’s Manual, 4.6.2.2 Instruction Cache Block Invalidate (icbi)):

    ```
    sync
    icbi (to any address)
    isync
    ```

PPC\_FEATURE\_ARCH\_2\_05
:   The processor supports the v2.05 userlevel architecture. Processors
    supporting later architectures DO NOT set this feature.

PPC\_FEATURE\_PA6T
:   The processor is PA6T.

PPC\_FEATURE\_HAS\_DFP
:   DFP facility is available.

PPC\_FEATURE\_POWER6\_EXT
:   The processor is POWER6.

PPC\_FEATURE\_ARCH\_2\_06
:   The processor supports the v2.06 userlevel architecture. Processors
    supporting later architectures also set this feature.

PPC\_FEATURE\_HAS\_VSX
:   VSX facility is available.

PPC\_FEATURE\_PSERIES\_PERFMON\_COMPAT
:   The processor supports architected PMU events in the range 0xE0-0xFF.

PPC\_FEATURE\_TRUE\_LE
:   The processor supports true little-endian mode.

PPC\_FEATURE\_PPC\_LE
:   The processor supports “PowerPC Little-Endian”, that uses address
    munging to make storage access appear to be little-endian, but the
    data is stored in a different format that is unsuitable to be
    accessed by other agents not running in this mode.

## 6. The HWCAPs exposed in AT\_HWCAP2

PPC\_FEATURE2\_ARCH\_2\_07
:   The processor supports the v2.07 userlevel architecture. Processors
    supporting later architectures also set this feature.

PPC\_FEATURE2\_HTM
:   Transactional Memory feature is available.

PPC\_FEATURE2\_DSCR
:   DSCR facility is available.

PPC\_FEATURE2\_EBB
:   EBB facility is available.

PPC\_FEATURE2\_ISEL
:   isel instruction is available. This is superseded by ARCH\_2\_07 and
    later.

PPC\_FEATURE2\_TAR
:   TAR facility is available.

PPC\_FEATURE2\_VEC\_CRYPTO
:   v2.07 crypto instructions are available.

PPC\_FEATURE2\_HTM\_NOSC
:   System calls fail if called in a transactional state, see
    [Power Architecture 64-bit Linux system call ABI](syscall64-abi.html)

PPC\_FEATURE2\_ARCH\_3\_00
:   The processor supports the v3.0B / v3.0C userlevel architecture. Processors
    supporting later architectures also set this feature.

PPC\_FEATURE2\_HAS\_IEEE128
:   IEEE 128-bit binary floating point is supported with VSX
    quad-precision instructions and data types.

PPC\_FEATURE2\_DARN
:   darn instruction is available.

PPC\_FEATURE2\_SCV
:   The scv 0 instruction may be used for system calls, see
    [Power Architecture 64-bit Linux system call ABI](syscall64-abi.html).

PPC\_FEATURE2\_HTM\_NO\_SUSPEND
:   A limited Transactional Memory facility that does not support suspend is
    available, see [Transactional Memory support](transactional_memory.html).

PPC\_FEATURE2\_ARCH\_3\_1
:   The processor supports the v3.1 userlevel architecture. Processors
    supporting later architectures also set this feature.

PPC\_FEATURE2\_MMA
:   MMA facility is available.
