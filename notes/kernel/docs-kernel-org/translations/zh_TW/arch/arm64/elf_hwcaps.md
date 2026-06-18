# ARM64 ELF hwcaps

> 출처(원문): https://docs.kernel.org/translations/zh_TW/arch/arm64/elf_hwcaps.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Warning

此文件的目的是爲讓中文讀者更容易閱讀和理解，而不是作爲一個分支。因此，
如果您對此文件有任何意見或改動，請先嘗試更新原始英文文件。如果要更改或
修正某處翻譯文件，請將意見或補丁發送給維護者（聯繫方式見下）。

Note

如果您發現本文檔與原始文件有任何不同或者有翻譯問題，請聯繫該文件的譯者，
或者發送電子郵件給胡皓文以獲取幫助：<[2023002089@link.tyut.edu.cn](mailto:2023002089%40link.tyut.edu.cn)>。

Original:
:   [Documentation/arch/arm64/elf\_hwcaps.rst](../../../../arch/arm64/elf_hwcaps.html#elf-hwcaps-index)

Translator: Bailu Lin <[bailu.lin@vivo.com](mailto:bailu.lin%40vivo.com)>
:   Hu Haowen <[2023002089@link.tyut.edu.cn](mailto:2023002089%40link.tyut.edu.cn)>

# ARM64 ELF hwcaps

這篇文檔描述了 arm64 ELF hwcaps 的用法和語義。

## 1. 簡介

有些硬件或軟件功能僅在某些 CPU 實現上和/或在具體某個內核配置上可用，但
對於處於 EL0 的用戶空間代碼沒有可用的架構發現機制。內核通過在輔助向量表
公開一組稱爲 hwcaps 的標誌而把這些功能暴露給用戶空間。

用戶空間軟件可以通過獲取輔助向量的 AT\_HWCAP 或 AT\_HWCAP2 條目來測試功能，
並測試是否設置了相關標誌，例如:

```
bool floating_point_is_present(void)
{
        unsigned long hwcaps = getauxval(AT_HWCAP);
        if (hwcaps & HWCAP_FP)
                return true;

        return false;
}
```

如果軟件依賴於 hwcap 描述的功能，在嘗試使用該功能前則應檢查相關的 hwcap
標誌以驗證該功能是否存在。

不能通過其他方式探查這些功能。當一個功能不可用時，嘗試使用它可能導致不可
預測的行爲，並且無法保證能確切的知道該功能不可用，例如 SIGILL。

## 2. Hwcaps 的說明

大多數 hwcaps 旨在說明通過架構 ID 寄存器(處於 EL0 的用戶空間代碼無法訪問)
描述的功能的存在。這些 hwcap 通過 ID 寄存器字段定義，並且應根據 ARM 體系
結構參考手冊（ARM ARM）中定義的字段來解釋說明。

這些 hwcaps 以下面的形式描述:

```
idreg.field == val 表示有某個功能。
```

當 idreg.field 中有 val 時，hwcaps 表示 ARM ARM 定義的功能是有效的，但是
並不是說要完全和 val 相等，也不是說 idreg.field 描述的其他功能就是缺失的。

其他 hwcaps 可能表明無法僅由 ID 寄存器描述的功能的存在。這些 hwcaps 可能
沒有被 ID 寄存器描述，需要參考其他文檔。

## 3. AT\_HWCAP 中揭示的 hwcaps

HWCAP\_FP
:   ID\_AA64PFR0\_EL1.FP == 0b0000 表示有此功能。

HWCAP\_ASIMD
:   ID\_AA64PFR0\_EL1.AdvSIMD == 0b0000 表示有此功能。

HWCAP\_EVTSTRM
:   通用計時器頻率配置爲大約100KHz以生成事件。

HWCAP\_AES
:   ID\_AA64ISAR0\_EL1.AES == 0b0001 表示有此功能。

HWCAP\_PMULL
:   ID\_AA64ISAR0\_EL1.AES == 0b0010 表示有此功能。

HWCAP\_SHA1
:   ID\_AA64ISAR0\_EL1.SHA1 == 0b0001 表示有此功能。

HWCAP\_SHA2
:   ID\_AA64ISAR0\_EL1.SHA2 == 0b0001 表示有此功能。

HWCAP\_CRC32
:   ID\_AA64ISAR0\_EL1.CRC32 == 0b0001 表示有此功能。

HWCAP\_ATOMICS
:   ID\_AA64ISAR0\_EL1.Atomic == 0b0010 表示有此功能。

HWCAP\_FPHP
:   ID\_AA64PFR0\_EL1.FP == 0b0001 表示有此功能。

HWCAP\_ASIMDHP
:   ID\_AA64PFR0\_EL1.AdvSIMD == 0b0001 表示有此功能。

HWCAP\_CPUID
:   根據 [ARM64 CPU Feature Registers](../../../../arch/arm64/cpu-feature-registers.html) 描述，EL0 可以訪問
    某些 ID 寄存器。

    這些 ID 寄存器可能表示功能的可用性。

HWCAP\_ASIMDRDM
:   ID\_AA64ISAR0\_EL1.RDM == 0b0001 表示有此功能。

HWCAP\_JSCVT
:   ID\_AA64ISAR1\_EL1.JSCVT == 0b0001 表示有此功能。

HWCAP\_FCMA
:   ID\_AA64ISAR1\_EL1.FCMA == 0b0001 表示有此功能。

HWCAP\_LRCPC
:   ID\_AA64ISAR1\_EL1.LRCPC == 0b0001 表示有此功能。

HWCAP\_DCPOP
:   ID\_AA64ISAR1\_EL1.DPB == 0b0001 表示有此功能。

HWCAP\_SHA3
:   ID\_AA64ISAR0\_EL1.SHA3 == 0b0001 表示有此功能。

HWCAP\_SM3
:   ID\_AA64ISAR0\_EL1.SM3 == 0b0001 表示有此功能。

HWCAP\_SM4
:   ID\_AA64ISAR0\_EL1.SM4 == 0b0001 表示有此功能。

HWCAP\_ASIMDDP
:   ID\_AA64ISAR0\_EL1.DP == 0b0001 表示有此功能。

HWCAP\_SHA512
:   ID\_AA64ISAR0\_EL1.SHA2 == 0b0010 表示有此功能。

HWCAP\_SVE
:   ID\_AA64PFR0\_EL1.SVE == 0b0001 表示有此功能。

HWCAP\_ASIMDFHM
:   ID\_AA64ISAR0\_EL1.FHM == 0b0001 表示有此功能。

HWCAP\_DIT
:   ID\_AA64PFR0\_EL1.DIT == 0b0001 表示有此功能。

HWCAP\_USCAT
:   ID\_AA64MMFR2\_EL1.AT == 0b0001 表示有此功能。

HWCAP\_ILRCPC
:   ID\_AA64ISAR1\_EL1.LRCPC == 0b0010 表示有此功能。

HWCAP\_FLAGM
:   ID\_AA64ISAR0\_EL1.TS == 0b0001 表示有此功能。

HWCAP\_SSBS
:   ID\_AA64PFR1\_EL1.SSBS == 0b0010 表示有此功能。

HWCAP\_SB
:   ID\_AA64ISAR1\_EL1.SB == 0b0001 表示有此功能。

HWCAP\_PACA
:   如 [Pointer authentication in AArch64 Linux](../../../../arch/arm64/pointer-authentication.html) 所描述，
    ID\_AA64ISAR1\_EL1.APA == 0b0001 或 ID\_AA64ISAR1\_EL1.API == 0b0001
    表示有此功能。

HWCAP\_PACG
:   如 [Pointer authentication in AArch64 Linux](../../../../arch/arm64/pointer-authentication.html) 所描述，
    ID\_AA64ISAR1\_EL1.GPA == 0b0001 或 ID\_AA64ISAR1\_EL1.GPI == 0b0001
    表示有此功能。

HWCAP2\_DCPODP

> ID\_AA64ISAR1\_EL1.DPB == 0b0010 表示有此功能。

HWCAP2\_SVE2

> ID\_AA64ZFR0\_EL1.SVEVer == 0b0001 表示有此功能。

HWCAP2\_SVEAES

> ID\_AA64ZFR0\_EL1.AES == 0b0001 表示有此功能。

HWCAP2\_SVEPMULL

> ID\_AA64ZFR0\_EL1.AES == 0b0010 表示有此功能。

HWCAP2\_SVEBITPERM

> ID\_AA64ZFR0\_EL1.BitPerm == 0b0001 表示有此功能。

HWCAP2\_SVESHA3

> ID\_AA64ZFR0\_EL1.SHA3 == 0b0001 表示有此功能。

HWCAP2\_SVESM4

> ID\_AA64ZFR0\_EL1.SM4 == 0b0001 表示有此功能。

HWCAP2\_FLAGM2

> ID\_AA64ISAR0\_EL1.TS == 0b0010 表示有此功能。

HWCAP2\_FRINT

> ID\_AA64ISAR1\_EL1.FRINTTS == 0b0001 表示有此功能。

HWCAP2\_SVEI8MM

> ID\_AA64ZFR0\_EL1.I8MM == 0b0001 表示有此功能。

HWCAP2\_SVEF32MM

> ID\_AA64ZFR0\_EL1.F32MM == 0b0001 表示有此功能。

HWCAP2\_SVEF64MM

> ID\_AA64ZFR0\_EL1.F64MM == 0b0001 表示有此功能。

HWCAP2\_SVEBF16

> ID\_AA64ZFR0\_EL1.BF16 == 0b0001 表示有此功能。

HWCAP2\_I8MM

> ID\_AA64ISAR1\_EL1.I8MM == 0b0001 表示有此功能。

HWCAP2\_BF16

> ID\_AA64ISAR1\_EL1.BF16 == 0b0001 表示有此功能。

HWCAP2\_DGH

> ID\_AA64ISAR1\_EL1.DGH == 0b0001 表示有此功能。

HWCAP2\_RNG

> ID\_AA64ISAR0\_EL1.RNDR == 0b0001 表示有此功能。

HWCAP2\_BTI

> ID\_AA64PFR0\_EL1.BT == 0b0001 表示有此功能。

## 4. 未使用的 AT\_HWCAP 位

爲了與用戶空間交互，內核保證 AT\_HWCAP 的第62、63位將始終返回0。
