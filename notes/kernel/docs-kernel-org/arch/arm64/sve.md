# Scalable Vector Extension support for AArch64 Linux

> 출처(원문): https://docs.kernel.org/arch/arm64/sve.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Scalable Vector Extension support for AArch64 Linux

Author: Dave Martin <[Dave.Martin@arm.com](mailto:Dave.Martin%40arm.com)>

Date: 4 August 2017

This document outlines briefly the interface provided to userspace by Linux in
order to support use of the ARM Scalable Vector Extension (SVE), including
interactions with Streaming SVE mode added by the Scalable Matrix Extension
(SME).

This is an outline of the most important features and issues only and not
intended to be exhaustive.

This document does not aim to describe the SVE architecture or programmer’s
model. To aid understanding, a minimal description of relevant programmer’s
model features for SVE is included in Appendix A.

## 1. General

* SVE registers Z0..Z31, P0..P15 and FFR and the current vector length VL, are
  tracked per-thread.
* In streaming mode FFR is not accessible unless HWCAP2\_SME\_FA64 is present
  in the system, when it is not supported and these interfaces are used to
  access streaming mode FFR is read and written as zero.
* The presence of SVE is reported to userspace via HWCAP\_SVE in the aux vector
  AT\_HWCAP entry. Presence of this flag implies the presence of the SVE
  instructions and registers, and the Linux-specific system interfaces
  described in this document. SVE is reported in /proc/cpuinfo as “sve”.
* Support for the execution of SVE instructions in userspace can also be
  detected by reading the CPU ID register ID\_AA64PFR0\_EL1 using an MRS
  instruction, and checking that the value of the SVE field is nonzero. [3]

  It does not guarantee the presence of the system interfaces described in the
  following sections: software that needs to verify that those interfaces are
  present must check for HWCAP\_SVE instead.
* On hardware that supports the SVE2 extensions, HWCAP2\_SVE2 will also
  be reported in the AT\_HWCAP2 aux vector entry. In addition to this,
  optional extensions to SVE2 may be reported by the presence of:

  > HWCAP2\_SVE2
  > HWCAP2\_SVEAES
  > HWCAP2\_SVEPMULL
  > HWCAP2\_SVEBITPERM
  > HWCAP2\_SVESHA3
  > HWCAP2\_SVESM4
  > HWCAP2\_SVE2P1

  This list may be extended over time as the SVE architecture evolves.

  These extensions are also reported via the CPU ID register ID\_AA64ZFR0\_EL1,
  which userspace can read using an MRS instruction. See [ARM64 ELF hwcaps](elf_hwcaps.html) and
  [ARM64 CPU Feature Registers](cpu-feature-registers.html) for details.
* On hardware that supports the SME extensions, HWCAP2\_SME will also be
  reported in the AT\_HWCAP2 aux vector entry. Among other things SME adds
  streaming mode which provides a subset of the SVE feature set using a
  separate SME vector length and the same Z/V registers. See [Scalable Matrix Extension support for AArch64 Linux](sme.html)
  for more details.
* Debuggers should restrict themselves to interacting with the target via the
  NT\_ARM\_SVE regset. The recommended way of detecting support for this regset
  is to connect to a target process first and then attempt a
  ptrace(PTRACE\_GETREGSET, pid, NT\_ARM\_SVE, &iov). Note that when SME is
  present and streaming SVE mode is in use the FPSIMD subset of registers
  will be read via NT\_ARM\_SVE and NT\_ARM\_SVE writes will exit streaming mode
  in the target.
* Whenever SVE scalable register values (Zn, Pn, FFR) are exchanged in memory
  between userspace and the kernel, the register value is encoded in memory in
  an endianness-invariant layout, with bits [(8 \* i + 7) : (8 \* i)] encoded at
  byte offset i from the start of the memory representation. This affects for
  example the signal frame (`struct sve_context`) and ptrace interface
  (`struct user_sve_header`) and associated data.

  Beware that on big-endian systems this results in a different byte order than
  for the FPSIMD V-registers, which are stored as single host-endian 128-bit
  values, with bits [(127 - 8 \* i) : (120 - 8 \* i)] of the register encoded at
  byte offset i. (`struct fpsimd_context`, `struct user_fpsimd_state`).

## 2. Vector length terminology

The size of an SVE vector (Z) register is referred to as the “vector length”.

To avoid confusion about the units used to express vector length, the kernel
adopts the following conventions:

* Vector length (VL) = size of a Z-register in bytes
* Vector quadwords (VQ) = size of a Z-register in units of 128 bits

(So, VL = 16 \* VQ.)

The VQ convention is used where the underlying granularity is important, such
as in data structure definitions. In most other situations, the VL convention
is used. This is consistent with the meaning of the “VL” pseudo-register in
the SVE instruction set architecture.

## 3. System call behaviour

* On syscall, V0..V31 are preserved (as without SVE). Thus, bits [127:0] of
  Z0..Z31 are preserved. All other bits of Z0..Z31, and all of P0..P15 and FFR
  become zero on return from a syscall.
* The SVE registers are not used to pass arguments to or receive results from
  any syscall.
* All other SVE state of a thread, including the currently configured vector
  length, the state of the PR\_SVE\_VL\_INHERIT flag, and the deferred vector
  length (if any), is preserved across all syscalls, subject to the specific
  exceptions for execve() described in section 6.

  In particular, on return from a fork() or clone(), the parent and new child
  process or thread share identical SVE configuration, matching that of the
  parent before the call.

## 4. Signal handling

* A new signal frame record sve\_context encodes the SVE registers on signal
  delivery. [1]
* This record is supplementary to fpsimd\_context. The FPSR and FPCR registers
  are only present in fpsimd\_context. For convenience, the content of V0..V31
  is duplicated between sve\_context and fpsimd\_context.
* The record contains a flag field which includes a flag SVE\_SIG\_FLAG\_SM which
  if set indicates that the thread is in streaming mode and the vector length
  and register data (if present) describe the streaming SVE data and vector
  length.
* The signal frame record for SVE always contains basic metadata, in particular
  the thread’s vector length (in sve\_context.vl).
* The SVE registers may or may not be included in the record, depending on
  whether the registers are live for the thread. The registers are present if
  and only if:
  sve\_context.head.size >= SVE\_SIG\_CONTEXT\_SIZE(sve\_vq\_from\_vl(sve\_context.vl)).
* If the registers are present, the remainder of the record has a vl-dependent
  size and layout. Macros SVE\_SIG\_\* are defined [1] to facilitate access to
  the members.
* Each scalable register (Zn, Pn, FFR) is stored in an endianness-invariant
  layout, with bits [(8 \* i + 7) : (8 \* i)] stored at byte offset i from the
  start of the register’s representation in memory.
* If the SVE context is too big to fit in sigcontext.\_\_reserved[], then extra
  space is allocated on the stack, an extra\_context record is written in
  \_\_reserved[] referencing this space. sve\_context is then written in the
  extra space. Refer to [1] for further details about this mechanism.

## 5. Signal return

When returning from a signal handler:

* If there is no sve\_context record in the signal frame, or if the record is
  present but contains no register data as described in the previous section,
  then the SVE registers/bits become non-live and take unspecified values.
* If sve\_context is present in the signal frame and contains full register
  data, the SVE registers become live and are populated with the specified
  data. However, for backward compatibility reasons, bits [127:0] of Z0..Z31
  are always restored from the corresponding members of fpsimd\_context.vregs[]
  and not from sve\_context. The remaining bits are restored from sve\_context.
* Inclusion of fpsimd\_context in the signal frame remains mandatory,
  irrespective of whether sve\_context is present or not.
* The vector length cannot be changed via signal return. If sve\_context.vl in
  the signal frame does not match the current vector length, the signal return
  attempt is treated as illegal, resulting in a forced SIGSEGV.
* It is permitted to enter or leave streaming mode by setting or clearing
  the SVE\_SIG\_FLAG\_SM flag but applications should take care to ensure that
  when doing so sve\_context.vl and any register data are appropriate for the
  vector length in the new mode.

## 6. prctl extensions

Some new `prctl()` calls are added to allow programs to manage the SVE vector
length:

prctl(PR\_SVE\_SET\_VL, unsigned long arg)

> Sets the vector length of the calling thread and related flags, where
> arg == vl | flags. Other threads of the calling process are unaffected.
>
> vl is the desired vector length, where sve\_vl\_valid(vl) must be true.
>
> flags:
>
> > PR\_SVE\_VL\_INHERIT
> >
> > > Inherit the current vector length across execve(). Otherwise, the
> > > vector length is reset to the system default at execve(). (See
> > > Section 9.)
> >
> > PR\_SVE\_SET\_VL\_ONEXEC
> >
> > > Defer the requested vector length change until the next execve()
> > > performed by this thread.
> > >
> > > The effect is equivalent to implicit execution of the following
> > > call immediately after the next execve() (if any) by the thread:
> > >
> > > > prctl(PR\_SVE\_SET\_VL, arg & ~PR\_SVE\_SET\_VL\_ONEXEC)
> > >
> > > This allows launching of a new program with a different vector
> > > length, while avoiding runtime side effects in the caller.
> > >
> > > Without PR\_SVE\_SET\_VL\_ONEXEC, the requested change takes effect
> > > immediately.
>
> Return value: a nonnegative on success, or a negative value on error:
> :   EINVAL: SVE not supported, invalid vector length requested, or
>     :   invalid flags.
>
> On success:
>
> * Either the calling thread’s vector length or the deferred vector length
>   to be applied at the next execve() by the thread (dependent on whether
>   PR\_SVE\_SET\_VL\_ONEXEC is present in arg), is set to the largest value
>   supported by the system that is less than or equal to vl. If vl ==
>   SVE\_VL\_MAX, the value set will be the largest value supported by the
>   system.
> * Any previously outstanding deferred vector length change in the calling
>   thread is cancelled.
> * The returned value describes the resulting configuration, encoded as for
>   PR\_SVE\_GET\_VL. The vector length reported in this value is the new
>   current vector length for this thread if PR\_SVE\_SET\_VL\_ONEXEC was not
>   present in arg; otherwise, the reported vector length is the deferred
>   vector length that will be applied at the next execve() by the calling
>   thread.
> * Changing the vector length causes all of P0..P15, FFR and all bits of
>   Z0..Z31 except for Z0 bits [127:0] .. Z31 bits [127:0] to become
>   unspecified. Calling PR\_SVE\_SET\_VL with vl equal to the thread’s current
>   vector length, or calling PR\_SVE\_SET\_VL with the PR\_SVE\_SET\_VL\_ONEXEC
>   flag, does not constitute a change to the vector length for this purpose.

prctl(PR\_SVE\_GET\_VL)

> Gets the vector length of the calling thread.
>
> The following flag may be OR-ed into the result:
>
> > PR\_SVE\_VL\_INHERIT
> >
> > > Vector length will be inherited across execve().
>
> There is no way to determine whether there is an outstanding deferred
> vector length change (which would only normally be the case between a
> fork() or `vfork()` and the corresponding execve() in typical use).
>
> To extract the vector length from the result, bitwise and it with
> PR\_SVE\_VL\_LEN\_MASK.
>
> Return value: a nonnegative value on success, or a negative value on error:
> :   EINVAL: SVE not supported.

## 7. ptrace extensions

* New regsets NT\_ARM\_SVE and NT\_ARM\_SSVE are defined for use with
  PTRACE\_GETREGSET and PTRACE\_SETREGSET. NT\_ARM\_SSVE describes the
  streaming mode SVE registers and NT\_ARM\_SVE describes the
  non-streaming mode SVE registers.

  In this description a register set is referred to as being “live” when
  the target is in the appropriate streaming or non-streaming mode and is
  using data beyond the subset shared with the FPSIMD Vn registers.

  Refer to [2] for definitions.

The regset data starts with `struct user_sve_header`, containing:

> size
>
> > Size of the complete regset, in bytes.
> > This depends on vl and possibly on other things in the future.
> >
> > If a call to PTRACE\_GETREGSET requests less data than the value of
> > size, the caller can allocate a larger buffer and retry in order to
> > read the complete regset.
>
> max\_size
>
> > Maximum size in bytes that the regset can grow to for the target
> > thread. The regset won’t grow bigger than this even if the target
> > thread changes its vector length etc.
>
> vl
>
> > Target thread’s current vector length, in bytes.
>
> max\_vl
>
> > Maximum possible vector length for the target thread.
>
> flags
>
> > at most one of
> >
> > > SVE\_PT\_REGS\_FPSIMD
> > >
> > > > SVE registers are not live (GETREGSET) or are to be made
> > > > non-live (SETREGSET).
> > > >
> > > > The payload is of type `struct user_fpsimd_state`, with the same
> > > > meaning as for NT\_PRFPREG, starting at offset
> > > > SVE\_PT\_FPSIMD\_OFFSET from the start of user\_sve\_header.
> > > >
> > > > Extra data might be appended in the future: the size of the
> > > > payload should be obtained using SVE\_PT\_FPSIMD\_SIZE(vq, flags).
> > > >
> > > > vq should be obtained using sve\_vq\_from\_vl(vl).
> > > >
> > > > or
> > >
> > > SVE\_PT\_REGS\_SVE
> > >
> > > > SVE registers are live (GETREGSET) or are to be made live
> > > > (SETREGSET).
> > > >
> > > > The payload contains the SVE register data, starting at offset
> > > > SVE\_PT\_SVE\_OFFSET from the start of user\_sve\_header, and with
> > > > size SVE\_PT\_SVE\_SIZE(vq, flags);
> >
> > ... OR-ed with zero or more of the following flags, which have the same
> > meaning and behaviour as the corresponding PR\_SET\_VL\_\* flags:
> >
> > > SVE\_PT\_VL\_INHERIT
> > >
> > > SVE\_PT\_VL\_ONEXEC (SETREGSET only).
> >
> > If neither FPSIMD nor SVE flags are provided then no register
> > payload is available, this is only possible when SME is implemented.

* The effects of changing the vector length and/or flags are equivalent to
  those documented for PR\_SVE\_SET\_VL.

  The caller must make a further GETREGSET call if it needs to know what VL is
  actually set by SETREGSET, unless is it known in advance that the requested
  VL is supported.
* In the SVE\_PT\_REGS\_SVE case, the size and layout of the payload depends on
  the header fields. The SVE\_PT\_SVE\_\*() macros are provided to facilitate
  access to the members.
* In either case, for SETREGSET it is permissible to omit the payload, in which
  case only the vector length and flags are changed (along with any
  consequences of those changes).
* In systems supporting SME when in streaming mode a GETREGSET for
  NT\_REG\_SVE will return only the user\_sve\_header with no register data,
  similarly a GETREGSET for NT\_REG\_SSVE will not return any register data
  when not in streaming mode.
* A GETREGSET for NT\_ARM\_SSVE will never return SVE\_PT\_REGS\_FPSIMD.
* For SETREGSET, if an SVE\_PT\_REGS\_SVE payload is present and the
  requested VL is not supported, the effect will be the same as if the
  payload were omitted, except that an EIO error is reported. No
  attempt is made to translate the payload data to the correct layout
  for the vector length actually set. The thread’s FPSIMD state is
  preserved, but the remaining bits of the SVE registers become
  unspecified. It is up to the caller to translate the payload layout
  for the actual VL and retry.
* Where SME is implemented it is not possible to GETREGSET the register
  state for normal SVE when in streaming mode, nor the streaming mode
  register state when in normal mode, regardless of the implementation defined
  behaviour of the hardware for sharing data between the two modes.
* Any SETREGSET of NT\_ARM\_SVE will exit streaming mode if the target was in
  streaming mode and any SETREGSET of NT\_ARM\_SSVE will enter streaming mode
  if the target was not in streaming mode.
* On systems that do not support SVE it is permitted to use SETREGSET to
  write SVE\_PT\_REGS\_FPSIMD formatted data via NT\_ARM\_SVE, in this case the
  vector length should be specified as 0. This allows streaming mode to be
  disabled on systems with SME but not SVE.
* If any register data is provided along with SVE\_PT\_VL\_ONEXEC then the
  registers data will be interpreted with the current vector length, not
  the vector length configured for use on exec.
* The effect of writing a partial, incomplete payload is unspecified.

## 8. ELF coredump extensions

* NT\_ARM\_SVE and NT\_ARM\_SSVE notes will be added to each coredump for
  each thread of the dumped process. The contents will be equivalent to the
  data that would have been read if a PTRACE\_GETREGSET of the corresponding
  type were executed for each thread when the coredump was generated.

## 9. System runtime configuration

* To mitigate the ABI impact of expansion of the signal frame, a policy
  mechanism is provided for administrators, distro maintainers and developers
  to set the default vector length for userspace processes:

/proc/sys/abi/sve\_default\_vector\_length

> Writing the text representation of an integer to this file sets the system
> default vector length to the specified value rounded to a supported value
> using the same rules as for setting vector length via PR\_SVE\_SET\_VL.
>
> The result can be determined by reopening the file and reading its
> contents.
>
> At boot, the default vector length is initially set to 64 or the maximum
> supported vector length, whichever is smaller. This determines the initial
> vector length of the init process (PID 1).
>
> Reading this file returns the current system default vector length.

* At every execve() call, the new vector length of the new process is set to
  the system default vector length, unless

  > + PR\_SVE\_VL\_INHERIT (or equivalently SVE\_PT\_VL\_INHERIT) is set for the
  >   calling thread, or
  > + a deferred vector length change is pending, established via the
  >   PR\_SVE\_SET\_VL\_ONEXEC flag (or SVE\_PT\_VL\_ONEXEC).
* Modifying the system default vector length does not affect the vector length
  of any existing process or thread that does not make an execve() call.

## 10. Perf extensions

* The arm64 specific DWARF standard [5] added the VG (Vector Granule) register
  at index 46. This register is used for DWARF unwinding when variable length
  SVE registers are pushed onto the stack.
* Its value is equivalent to the current SVE vector length (VL) in bits divided
  by 64.
* The value is included in Perf samples in the regs[46] field if
  PERF\_SAMPLE\_REGS\_USER is set and the sample\_regs\_user mask has bit 46 set.
* The value is the current value at the time the sample was taken, and it can
  change over time.
* If the system doesn’t support SVE when perf\_event\_open is called with these
  settings, the event will fail to open.

### Appendix A. SVE programmer’s model (informative)

This section provides a minimal description of the additions made by SVE to the
ARMv8-A programmer’s model that are relevant to this document.

Note: This section is for information only and not intended to be complete or
to replace any architectural specification.

## A.1. Registers

In A64 state, SVE adds the following:

* 32 8VL-bit vector registers Z0..Z31
  For each Zn, Zn bits [127:0] alias the ARMv8-A vector register Vn.

  A register write using a Vn register name zeros all bits of the corresponding
  Zn except for bits [127:0].
* 16 VL-bit predicate registers P0..P15
* 1 VL-bit special-purpose predicate register FFR (the “first-fault register”)
* a VL “pseudo-register” that determines the size of each vector register

  The SVE instruction set architecture provides no way to write VL directly.
  Instead, it can be modified only by EL1 and above, by writing appropriate
  system registers.
* The value of VL can be configured at runtime by EL1 and above:
  16 <= VL <= VLmax, where VL must be a multiple of 16.
* The maximum vector length is determined by the hardware:
  16 <= VLmax <= 256.

  (The SVE architecture specifies 256, but permits future architecture
  revisions to raise this limit.)
* FPSR and FPCR are retained from ARMv8-A, and interact with SVE floating-point
  operations in a similar way to the way in which they interact with ARMv8
  floating-point operations:

  ```
       8VL-1                       128               0  bit index
      +----          ////            -----------------+
   Z0 |                               :       V0      |
    :                                          :
   Z7 |                               :       V7      |
   Z8 |                               :     * V8      |
    :                                       :  :
  Z15 |                               :     *V15      |
  Z16 |                               :      V16      |
    :                                          :
  Z31 |                               :      V31      |
      +----          ////            -----------------+
                                               31    0
       VL-1                  0                +-------+
      +----       ////      --+          FPSR |       |
   P0 |                       |               +-------+
    : |                       |         *FPCR |       |
  P15 |                       |               +-------+
      +----       ////      --+
  FFR |                       |               +-----+
      +----       ////      --+            VL |     |
                                              +-----+
  ```

(\*) callee-save:
:   This only applies to bits [63:0] of Z-/V-registers.
    FPCR contains callee-save and caller-save bits. See [4] for details.

## A.2. Procedure call standard

The ARMv8-A base procedure call standard is extended as follows with respect to
the additional SVE register state:

* All SVE register bits that are not shared with FP/SIMD are caller-save.
* Z8 bits [63:0] .. Z15 bits [63:0] are callee-save.

  This follows from the way these bits are mapped to V8..V15, which are caller-
  save in the base procedure call standard.

### Appendix B. ARMv8-A FP/SIMD programmer’s model

Note: This section is for information only and not intended to be complete or
to replace any architectural specification.

Refer to [4] for more information.

ARMv8-A defines the following floating-point / SIMD register state:

* 32 128-bit vector registers V0..V31
* 2 32-bit status/control registers FPSR, FPCR

```
      127           0  bit index
     +---------------+
  V0 |               |
   : :               :
  V7 |               |
* V8 |               |
:  : :               :
*V15 |               |
 V16 |               |
   : :               :
 V31 |               |
     +---------------+

              31    0
             +-------+
        FPSR |       |
             +-------+
       *FPCR |       |
             +-------+
```

(\*) callee-save:
:   This only applies to bits [63:0] of V-registers.
    FPCR contains a mixture of callee-save and caller-save bits.

### References

[1] arch/arm64/include/uapi/asm/sigcontext.h
:   AArch64 Linux signal ABI definitions

[2] arch/arm64/include/uapi/asm/ptrace.h
:   AArch64 Linux ptrace ABI definitions

[3] [ARM64 CPU Feature Registers](cpu-feature-registers.html)

[4] ARM IHI0055C
:   <http://infocenter.arm.com/help/topic/com.arm.doc.ihi0055c/IHI0055C_beta_aapcs64.pdf>
    <http://infocenter.arm.com/help/topic/com.arm.doc.subset.swdev.abi/index.html>
    Procedure Call Standard for the ARM 64-bit Architecture (AArch64)

[5] <https://github.com/ARM-software/abi-aa/blob/main/aadwarf64/aadwarf64.rst>
