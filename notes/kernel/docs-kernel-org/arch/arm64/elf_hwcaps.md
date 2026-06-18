# ARM64 ELF hwcaps

> 출처(원문): https://docs.kernel.org/arch/arm64/elf_hwcaps.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# ARM64 ELF hwcaps

This document describes the usage and semantics of the arm64 ELF hwcaps.

## 1. Introduction

Some hardware or software features are only available on some CPU
implementations, and/or with certain kernel configurations, but have no
architected discovery mechanism available to userspace code at EL0. The
kernel exposes the presence of these features to userspace through a set
of flags called hwcaps, exposed in the auxiliary vector.

Userspace software can test for features by acquiring the AT\_HWCAP,
AT\_HWCAP2 or AT\_HWCAP3 entry of the auxiliary vector, and testing
whether the relevant flags are set, e.g.:

```
bool floating_point_is_present(void)
{
        unsigned long hwcaps = getauxval(AT_HWCAP);
        if (hwcaps & HWCAP_FP)
                return true;

        return false;
}
```

Where software relies on a feature described by a hwcap, it should check
the relevant hwcap flag to verify that the feature is present before
attempting to make use of the feature.

Features cannot be probed reliably through other means. When a feature
is not available, attempting to use it may result in unpredictable
behaviour, and is not guaranteed to result in any reliable indication
that the feature is unavailable, such as a SIGILL.

## 2. Interpretation of hwcaps

The majority of hwcaps are intended to indicate the presence of features
which are described by architected ID registers inaccessible to
userspace code at EL0. These hwcaps are defined in terms of ID register
fields, and should be interpreted with reference to the definition of
these fields in the ARM Architecture Reference Manual (ARM ARM).

Such hwcaps are described below in the form:

```
Functionality implied by idreg.field == val.
```

Such hwcaps indicate the availability of functionality that the ARM ARM
defines as being present when idreg.field has value val, but do not
indicate that idreg.field is precisely equal to val, nor do they
indicate the absence of functionality implied by other values of
idreg.field.

Other hwcaps may indicate the presence of features which cannot be
described by ID registers alone. These may be described without
reference to ID registers, and may refer to other documentation.

## 3. The hwcaps exposed in AT\_HWCAP

HWCAP\_FP
:   Functionality implied by ID\_AA64PFR0\_EL1.FP == 0b0000.

HWCAP\_ASIMD
:   Functionality implied by ID\_AA64PFR0\_EL1.AdvSIMD == 0b0000.

HWCAP\_EVTSTRM
:   The generic timer is configured to generate events at a frequency of
    approximately 10KHz.

HWCAP\_AES
:   Functionality implied by ID\_AA64ISAR0\_EL1.AES == 0b0001.

HWCAP\_PMULL
:   Functionality implied by ID\_AA64ISAR0\_EL1.AES == 0b0010.

HWCAP\_SHA1
:   Functionality implied by ID\_AA64ISAR0\_EL1.SHA1 == 0b0001.

HWCAP\_SHA2
:   Functionality implied by ID\_AA64ISAR0\_EL1.SHA2 == 0b0001.

HWCAP\_CRC32
:   Functionality implied by ID\_AA64ISAR0\_EL1.CRC32 == 0b0001.

HWCAP\_ATOMICS
:   Functionality implied by ID\_AA64ISAR0\_EL1.Atomic == 0b0010.

HWCAP\_FPHP
:   Functionality implied by ID\_AA64PFR0\_EL1.FP == 0b0001.

HWCAP\_ASIMDHP
:   Functionality implied by ID\_AA64PFR0\_EL1.AdvSIMD == 0b0001.

HWCAP\_CPUID
:   EL0 access to certain ID registers is available, to the extent
    described by [ARM64 CPU Feature Registers](cpu-feature-registers.html).

    These ID registers may imply the availability of features.

HWCAP\_ASIMDRDM
:   Functionality implied by ID\_AA64ISAR0\_EL1.RDM == 0b0001.

HWCAP\_JSCVT
:   Functionality implied by ID\_AA64ISAR1\_EL1.JSCVT == 0b0001.

HWCAP\_FCMA
:   Functionality implied by ID\_AA64ISAR1\_EL1.FCMA == 0b0001.

HWCAP\_LRCPC
:   Functionality implied by ID\_AA64ISAR1\_EL1.LRCPC == 0b0001.

HWCAP\_DCPOP
:   Functionality implied by ID\_AA64ISAR1\_EL1.DPB == 0b0001.

HWCAP\_SHA3
:   Functionality implied by ID\_AA64ISAR0\_EL1.SHA3 == 0b0001.

HWCAP\_SM3
:   Functionality implied by ID\_AA64ISAR0\_EL1.SM3 == 0b0001.

HWCAP\_SM4
:   Functionality implied by ID\_AA64ISAR0\_EL1.SM4 == 0b0001.

HWCAP\_ASIMDDP
:   Functionality implied by ID\_AA64ISAR0\_EL1.DP == 0b0001.

HWCAP\_SHA512
:   Functionality implied by ID\_AA64ISAR0\_EL1.SHA2 == 0b0010.

HWCAP\_SVE
:   Functionality implied by ID\_AA64PFR0\_EL1.SVE == 0b0001.

HWCAP\_ASIMDFHM
:   Functionality implied by ID\_AA64ISAR0\_EL1.FHM == 0b0001.

HWCAP\_DIT
:   Functionality implied by ID\_AA64PFR0\_EL1.DIT == 0b0001.

HWCAP\_USCAT
:   Functionality implied by ID\_AA64MMFR2\_EL1.AT == 0b0001.

HWCAP\_ILRCPC
:   Functionality implied by ID\_AA64ISAR1\_EL1.LRCPC == 0b0010.

HWCAP\_FLAGM
:   Functionality implied by ID\_AA64ISAR0\_EL1.TS == 0b0001.

HWCAP\_SSBS
:   Functionality implied by ID\_AA64PFR1\_EL1.SSBS == 0b0010.

HWCAP\_SB
:   Functionality implied by ID\_AA64ISAR1\_EL1.SB == 0b0001.

HWCAP\_PACA
:   Functionality implied by ID\_AA64ISAR1\_EL1.APA == 0b0001 or
    ID\_AA64ISAR1\_EL1.API == 0b0001, as described by
    [Pointer authentication in AArch64 Linux](pointer-authentication.html).

HWCAP\_PACG
:   Functionality implied by ID\_AA64ISAR1\_EL1.GPA == 0b0001 or
    ID\_AA64ISAR1\_EL1.GPI == 0b0001, as described by
    [Pointer authentication in AArch64 Linux](pointer-authentication.html).

HWCAP\_GCS
:   Functionality implied by ID\_AA64PFR1\_EL1.GCS == 0b1, as
    described by [Guarded Control Stack support for AArch64 Linux](gcs.html).

HWCAP\_CMPBR
:   Functionality implied by ID\_AA64ISAR2\_EL1.CSSC == 0b0010.

HWCAP\_FPRCVT
:   Functionality implied by ID\_AA64ISAR3\_EL1.FPRCVT == 0b0001.

HWCAP\_F8MM8
:   Functionality implied by ID\_AA64FPFR0\_EL1.F8MM8 == 0b0001.

HWCAP\_F8MM4
:   Functionality implied by ID\_AA64FPFR0\_EL1.F8MM4 == 0b0001.

HWCAP\_SVE\_F16MM
:   Functionality implied by ID\_AA64PFR0\_EL1.SVE == 0b0001 and
    ID\_AA64ZFR0\_EL1.F16MM == 0b0001.

HWCAP\_SVE\_ELTPERM
:   Functionality implied by ID\_AA64PFR0\_EL1.SVE == 0b0001 and
    ID\_AA64ZFR0\_EL1.ELTPERM == 0b0001.

HWCAP\_SVE\_AES2
:   Functionality implied by ID\_AA64PFR0\_EL1.SVE == 0b0001 and
    ID\_AA64ZFR0\_EL1.AES == 0b0011.

HWCAP\_SVE\_BFSCALE
:   Functionality implied by ID\_AA64PFR0\_EL1.SVE == 0b0001 and
    ID\_AA64ZFR0\_EL1.B16B16 == 0b0010.

HWCAP\_SVE2P2
:   Functionality implied by ID\_AA64PFR0\_EL1.SVE == 0b0001 and
    ID\_AA64ZFR0\_EL1.SVEver == 0b0011.

HWCAP\_SME2P2
:   Functionality implied by ID\_AA64SMFR0\_EL1.SMEver == 0b0011.

HWCAP\_SME\_SBITPERM
:   Functionality implied by ID\_AA64SMFR0\_EL1.SBitPerm == 0b1.

HWCAP\_SME\_AES
:   Functionality implied by ID\_AA64SMFR0\_EL1.AES == 0b1.

HWCAP\_SME\_SFEXPA
:   Functionality implied by ID\_AA64SMFR0\_EL1.SFEXPA == 0b1.

HWCAP\_SME\_STMOP
:   Functionality implied by ID\_AA64SMFR0\_EL1.STMOP == 0b1.

HWCAP\_SME\_SMOP4
:   Functionality implied by ID\_AA64SMFR0\_EL1.SMOP4 == 0b1.

HWCAP2\_DCPODP
:   Functionality implied by ID\_AA64ISAR1\_EL1.DPB == 0b0010.

HWCAP2\_SVE2
:   Functionality implied by ID\_AA64PFR0\_EL1.SVE == 0b0001 and
    ID\_AA64ZFR0\_EL1.SVEver == 0b0001.

HWCAP2\_SVEAES
:   Functionality implied by ID\_AA64PFR0\_EL1.SVE == 0b0001 and
    ID\_AA64ZFR0\_EL1.AES == 0b0001.

HWCAP2\_SVEPMULL
:   Functionality implied by ID\_AA64PFR0\_EL1.SVE == 0b0001 and
    ID\_AA64ZFR0\_EL1.AES == 0b0010.

HWCAP2\_SVEBITPERM
:   Functionality implied by ID\_AA64PFR0\_EL1.SVE == 0b0001 and
    ID\_AA64ZFR0\_EL1.BitPerm == 0b0001.

HWCAP2\_SVESHA3
:   Functionality implied by ID\_AA64PFR0\_EL1.SVE == 0b0001 and
    ID\_AA64ZFR0\_EL1.SHA3 == 0b0001.

HWCAP2\_SVESM4
:   Functionality implied by ID\_AA64PFR0\_EL1.SVE == 0b0001 and
    ID\_AA64ZFR0\_EL1.SM4 == 0b0001.

HWCAP2\_FLAGM2
:   Functionality implied by ID\_AA64ISAR0\_EL1.TS == 0b0010.

HWCAP2\_FRINT
:   Functionality implied by ID\_AA64ISAR1\_EL1.FRINTTS == 0b0001.

HWCAP2\_SVEI8MM
:   Functionality implied by ID\_AA64PFR0\_EL1.SVE == 0b0001 and
    ID\_AA64ZFR0\_EL1.I8MM == 0b0001.

HWCAP2\_SVEF32MM
:   Functionality implied by ID\_AA64PFR0\_EL1.SVE == 0b0001 and
    ID\_AA64ZFR0\_EL1.F32MM == 0b0001.

HWCAP2\_SVEF64MM
:   Functionality implied by ID\_AA64PFR0\_EL1.SVE == 0b0001 and
    ID\_AA64ZFR0\_EL1.F64MM == 0b0001.

HWCAP2\_SVEBF16
:   Functionality implied by ID\_AA64PFR0\_EL1.SVE == 0b0001 and
    ID\_AA64ZFR0\_EL1.BF16 == 0b0001.

HWCAP2\_I8MM
:   Functionality implied by ID\_AA64ISAR1\_EL1.I8MM == 0b0001.

HWCAP2\_BF16
:   Functionality implied by ID\_AA64ISAR1\_EL1.BF16 == 0b0001.

HWCAP2\_DGH
:   Functionality implied by ID\_AA64ISAR1\_EL1.DGH == 0b0001.

HWCAP2\_RNG
:   Functionality implied by ID\_AA64ISAR0\_EL1.RNDR == 0b0001.

HWCAP2\_BTI
:   Functionality implied by ID\_AA64PFR1\_EL1.BT == 0b0001.

HWCAP2\_MTE
:   Functionality implied by ID\_AA64PFR1\_EL1.MTE == 0b0010, as described
    by [Memory Tagging Extension (MTE) in AArch64 Linux](memory-tagging-extension.html).

HWCAP2\_ECV
:   Functionality implied by ID\_AA64MMFR0\_EL1.ECV == 0b0001.

HWCAP2\_AFP
:   Functionality implied by ID\_AA64MMFR1\_EL1.AFP == 0b0001.

HWCAP2\_RPRES
:   Functionality implied by ID\_AA64ISAR2\_EL1.RPRES == 0b0001.

HWCAP2\_MTE3
:   Functionality implied by ID\_AA64PFR1\_EL1.MTE == 0b0011, as described
    by [Memory Tagging Extension (MTE) in AArch64 Linux](memory-tagging-extension.html).

HWCAP2\_SME
:   Functionality implied by ID\_AA64PFR1\_EL1.SME == 0b0001, as described
    by [Scalable Matrix Extension support for AArch64 Linux](sme.html).

HWCAP2\_SME\_I16I64
:   Functionality implied by ID\_AA64SMFR0\_EL1.I16I64 == 0b1111.

HWCAP2\_SME\_F64F64
:   Functionality implied by ID\_AA64SMFR0\_EL1.F64F64 == 0b1.

HWCAP2\_SME\_I8I32
:   Functionality implied by ID\_AA64SMFR0\_EL1.I8I32 == 0b1111.

HWCAP2\_SME\_F16F32
:   Functionality implied by ID\_AA64SMFR0\_EL1.F16F32 == 0b1.

HWCAP2\_SME\_B16F32
:   Functionality implied by ID\_AA64SMFR0\_EL1.B16F32 == 0b1.

HWCAP2\_SME\_F32F32
:   Functionality implied by ID\_AA64SMFR0\_EL1.F32F32 == 0b1.

HWCAP2\_SME\_FA64
:   Functionality implied by ID\_AA64SMFR0\_EL1.FA64 == 0b1.

HWCAP2\_WFXT
:   Functionality implied by ID\_AA64ISAR2\_EL1.WFXT == 0b0010.

HWCAP2\_EBF16
:   Functionality implied by ID\_AA64ISAR1\_EL1.BF16 == 0b0010.

HWCAP2\_SVE\_EBF16
:   Functionality implied by ID\_AA64PFR0\_EL1.SVE == 0b0001 and
    ID\_AA64ZFR0\_EL1.BF16 == 0b0010.

HWCAP2\_CSSC
:   Functionality implied by ID\_AA64ISAR2\_EL1.CSSC == 0b0001.

HWCAP2\_RPRFM
:   Functionality implied by ID\_AA64ISAR2\_EL1.RPRFM == 0b0001.

HWCAP2\_SVE2P1
:   Functionality implied by ID\_AA64PFR0\_EL1.SVE == 0b0001 and
    ID\_AA64ZFR0\_EL1.SVEver == 0b0010.

HWCAP2\_SME2
:   Functionality implied by ID\_AA64SMFR0\_EL1.SMEver == 0b0001.

HWCAP2\_SME2P1
:   Functionality implied by ID\_AA64SMFR0\_EL1.SMEver == 0b0010.

HWCAP2\_SMEI16I32
:   Functionality implied by ID\_AA64SMFR0\_EL1.I16I32 == 0b0101

HWCAP2\_SMEBI32I32
:   Functionality implied by ID\_AA64SMFR0\_EL1.BI32I32 == 0b1

HWCAP2\_SMEB16B16
:   Functionality implied by ID\_AA64SMFR0\_EL1.B16B16 == 0b1

HWCAP2\_SMEF16F16
:   Functionality implied by ID\_AA64SMFR0\_EL1.F16F16 == 0b1

HWCAP2\_MOPS
:   Functionality implied by ID\_AA64ISAR2\_EL1.MOPS == 0b0001.

HWCAP2\_HBC
:   Functionality implied by ID\_AA64ISAR2\_EL1.BC == 0b0001.

HWCAP2\_SVE\_B16B16
:   Functionality implied by ID\_AA64PFR0\_EL1.SVE == 0b0001 and
    ID\_AA64ZFR0\_EL1.B16B16 == 0b0001.

HWCAP2\_LRCPC3
:   Functionality implied by ID\_AA64ISAR1\_EL1.LRCPC == 0b0011.

HWCAP2\_LSE128
:   Functionality implied by ID\_AA64ISAR0\_EL1.Atomic == 0b0011.

HWCAP2\_FPMR
:   Functionality implied by ID\_AA64PFR2\_EL1.FMR == 0b0001.

HWCAP2\_LUT
:   Functionality implied by ID\_AA64ISAR2\_EL1.LUT == 0b0001.

HWCAP2\_FAMINMAX
:   Functionality implied by ID\_AA64ISAR3\_EL1.FAMINMAX == 0b0001.

HWCAP2\_F8CVT
:   Functionality implied by ID\_AA64FPFR0\_EL1.F8CVT == 0b1.

HWCAP2\_F8FMA
:   Functionality implied by ID\_AA64FPFR0\_EL1.F8FMA == 0b1.

HWCAP2\_F8DP4
:   Functionality implied by ID\_AA64FPFR0\_EL1.F8DP4 == 0b1.

HWCAP2\_F8DP2
:   Functionality implied by ID\_AA64FPFR0\_EL1.F8DP2 == 0b1.

HWCAP2\_F8E4M3
:   Functionality implied by ID\_AA64FPFR0\_EL1.F8E4M3 == 0b1.

HWCAP2\_F8E5M2
:   Functionality implied by ID\_AA64FPFR0\_EL1.F8E5M2 == 0b1.

HWCAP2\_SME\_LUTV2
:   Functionality implied by ID\_AA64SMFR0\_EL1.LUTv2 == 0b1.

HWCAP2\_SME\_F8F16
:   Functionality implied by ID\_AA64SMFR0\_EL1.F8F16 == 0b1.

HWCAP2\_SME\_F8F32
:   Functionality implied by ID\_AA64SMFR0\_EL1.F8F32 == 0b1.

HWCAP2\_SME\_SF8FMA
:   Functionality implied by ID\_AA64SMFR0\_EL1.SF8FMA == 0b1.

HWCAP2\_SME\_SF8DP4
:   Functionality implied by ID\_AA64SMFR0\_EL1.SF8DP4 == 0b1.

HWCAP2\_SME\_SF8DP2
:   Functionality implied by ID\_AA64SMFR0\_EL1.SF8DP2 == 0b1.

HWCAP2\_SME\_SF8DP4
:   Functionality implied by ID\_AA64SMFR0\_EL1.SF8DP4 == 0b1.

HWCAP2\_POE
:   Functionality implied by ID\_AA64MMFR3\_EL1.S1POE == 0b0001.

HWCAP3\_MTE\_FAR
:   Functionality implied by ID\_AA64PFR2\_EL1.MTEFAR == 0b0001.

HWCAP3\_MTE\_STORE\_ONLY
:   Functionality implied by ID\_AA64PFR2\_EL1.MTESTOREONLY == 0b0001.

HWCAP3\_LSFE
:   Functionality implied by ID\_AA64ISAR3\_EL1.LSFE == 0b0001

HWCAP3\_LS64
:   Functionality implied by ID\_AA64ISAR1\_EL1.LS64 == 0b0001. Note that
    the function of instruction ld64b/st64b requires support by CPU, system
    and target (device) memory location and HWCAP3\_LS64 implies the support
    of CPU. User should only use ld64b/st64b on supported target (device)
    memory location, otherwise fallback to the non-atomic alternatives.

## 4. Unused AT\_HWCAP bits

For interoperation with userspace, the kernel guarantees that bits 62
and 63 of AT\_HWCAP will always be returned as 0.
