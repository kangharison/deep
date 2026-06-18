# RSB-related mitigations

> 출처(원문): https://docs.kernel.org/admin-guide/hw-vuln/rsb.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# RSB-related mitigations

Warning

Please keep this document up-to-date, otherwise you will be
volunteered to update it and convert it to a very long comment in
bugs.c!

Since 2018 there have been many Spectre CVEs related to the Return Stack
Buffer (RSB) (sometimes referred to as the Return Address Stack (RAS) or
Return Address Predictor (RAP) on AMD).

Information about these CVEs and how to mitigate them is scattered
amongst a myriad of microarchitecture-specific documents.

This document attempts to consolidate all the relevant information in
once place and clarify the reasoning behind the current RSB-related
mitigations. It’s meant to be as concise as possible, focused only on
the current kernel mitigations: what are the RSB-related attack vectors
and how are they currently being mitigated?

It’s *not* meant to describe how the RSB mechanism operates or how the
exploits work. More details about those can be found in the references
below.

Rather, this is basically a glorified comment, but too long to actually
be one. So when the next CVE comes along, a kernel developer can
quickly refer to this as a refresher to see what we’re actually doing
and why.

At a high level, there are two classes of RSB attacks: RSB poisoning
(Intel and AMD) and RSB underflow (Intel only). They must each be
considered individually for each attack vector (and microarchitecture
where applicable).

---

## RSB poisoning (Intel and AMD)

### SpectreRSB

RSB poisoning is a technique used by SpectreRSB [[1]](#spectre-rsb) where
an attacker poisons an RSB entry to cause a victim’s return instruction
to speculate to an attacker-controlled address. This can happen when
there are unbalanced CALLs/RETs after a context switch or VMEXIT.

* All attack vectors can potentially be mitigated by flushing out any
  poisoned RSB entries using an RSB filling sequence
  [[2]](#intel-rsb-filling) [[3]](#amd-rsb-filling) when transitioning between
  untrusted and trusted domains. But this has a performance impact and
  should be avoided whenever possible.

  Danger

  **FIXME**: Currently we’re flushing 32 entries. However, some CPU
  models have more than 32 entries. The loop count needs to be
  increased for those. More detailed information is needed about RSB
  sizes.
* On context switch, the user->user mitigation requires ensuring the
  RSB gets filled or cleared whenever IBPB gets written [[4]](#cond-ibpb)
  during a context switch:

  + AMD:
    :   On Zen 4+, IBPB (or SBPB [[5]](#amd-sbpb) if used) clears the RSB.
        This is indicated by IBPB\_RET in CPUID [[6]](#amd-ibpb-rsb).

        On Zen < 4, the RSB filling sequence [[3]](#amd-rsb-filling) must be
        always be done in addition to IBPB [[7]](#amd-ibpb-no-rsb). This is
        indicated by X86\_BUG\_IBPB\_NO\_RET.
  + Intel:
    :   IBPB always clears the RSB:

        “Software that executed before the IBPB command cannot control
        the predicted targets of indirect branches executed after the
        command on the same logical processor. The term indirect branch
        in this context includes near return instructions, so these
        predicted targets may come from the RSB.” [[8]](#intel-ibpb-rsb)
* On context switch, user->kernel attacks are prevented by SMEP. User
  space can only insert user space addresses into the RSB. Even
  non-canonical addresses can’t be inserted due to the page gap at the
  end of the user canonical address space reserved by TASK\_SIZE\_MAX.
  A SMEP #PF at instruction fetch prevents the kernel from speculatively
  executing user space.

  + AMD:
    :   “Finally, branches that are predicted as ‘ret’ instructions get
        their predicted targets from the Return Address Predictor (RAP).
        AMD recommends software use a RAP stuffing sequence (mitigation
        V2-3 in [2]) and/or Supervisor Mode Execution Protection (SMEP)
        to ensure that the addresses in the RAP are safe for
        speculation. Collectively, we refer to these mitigations as “RAP
        Protection”.” [[9]](#amd-smep-rsb)
  + Intel:
    :   “On processors with enhanced IBRS, an RSB overwrite sequence may
        not suffice to prevent the predicted target of a near return
        from using an RSB entry created in a less privileged predictor
        mode. Software can prevent this by enabling SMEP (for
        transitions from user mode to supervisor mode) and by having
        IA32\_SPEC\_CTRL.IBRS set during VM exits.” [[10]](#intel-smep-rsb)
* On VMEXIT, guest->host attacks are mitigated by eIBRS (and PBRSB
  mitigation if needed):

  + AMD:
    :   “When Automatic IBRS is enabled, the internal return address
        stack used for return address predictions is cleared on VMEXIT.”
        [[11]](#amd-eibrs-vmexit)
  + Intel:
    :   “On processors with enhanced IBRS, an RSB overwrite sequence may
        not suffice to prevent the predicted target of a near return
        from using an RSB entry created in a less privileged predictor
        mode. Software can prevent this by enabling SMEP (for
        transitions from user mode to supervisor mode) and by having
        IA32\_SPEC\_CTRL.IBRS set during VM exits. Processors with
        enhanced IBRS still support the usage model where IBRS is set
        only in the OS/VMM for OSes that enable SMEP. To do this, such
        processors will ensure that guest behavior cannot control the
        RSB after a VM exit once IBRS is set, even if IBRS was not set
        at the time of the VM exit.” [[12]](#intel-eibrs-vmexit)

    Note that some Intel CPUs are susceptible to Post-barrier Return
    Stack Buffer Predictions (PBRSB) [[13]](#intel-pbrsb), where the last
    CALL from the guest can be used to predict the first unbalanced RET.
    In this case the PBRSB mitigation is needed in addition to eIBRS.

### AMD RETBleed / SRSO / Branch Type Confusion

On AMD, poisoned RSB entries can also be created by the AMD RETBleed
variant [[14]](#retbleed-paper) [[15]](#amd-btc) or by Speculative Return Stack
Overflow [[16]](#amd-srso) (Inception [[17]](#inception-paper)). The kernel
protects itself by replacing every RET in the kernel with a branch to a
single safe RET.

---

## RSB underflow (Intel only)

### RSB Alternate (RSBA) (“Intel Retbleed”)

Some Intel Skylake-generation CPUs are susceptible to the Intel variant
of RETBleed [[14]](#retbleed-paper) (Return Stack Buffer Underflow
[[18]](#intel-rsbu)). If a RET is executed when the RSB buffer is empty due
to mismatched CALLs/RETs or returning from a deep call stack, the branch
predictor can fall back to using the Branch Target Buffer (BTB). If a
user forces a BTB collision then the RET can speculatively branch to a
user-controlled address.

* Note that RSB filling doesn’t fully mitigate this issue. If there
  are enough unbalanced RETs, the RSB may still underflow and fall back
  to using a poisoned BTB entry.
* On context switch, user->user underflow attacks are mitigated by the
  conditional IBPB [[4]](#cond-ibpb) on context switch which effectively
  clears the BTB:

  + “The indirect branch predictor barrier (IBPB) is an indirect branch
    control mechanism that establishes a barrier, preventing software
    that executed before the barrier from controlling the predicted
    targets of indirect branches executed after the barrier on the same
    logical processor.” [[19]](#intel-ibpb-btb)
* On context switch and VMEXIT, user->kernel and guest->host RSB
  underflows are mitigated by IBRS or eIBRS:

  + “Enabling IBRS (including enhanced IBRS) will mitigate the “RSBU”
    attack demonstrated by the researchers. As previously documented,
    Intel recommends the use of enhanced IBRS, where supported. This
    includes any processor that enumerates RRSBA but not RRSBA\_DIS\_S.”
    [[18]](#intel-rsbu)

  However, note that eIBRS and IBRS do not mitigate intra-mode attacks.
  Like RRSBA below, this is mitigated by clearing the BHB on kernel
  entry.

  As an alternative to classic IBRS, call depth tracking (combined with
  retpolines) can be used to track kernel returns and fill the RSB when
  it gets close to being empty.

### Restricted RSB Alternate (RRSBA)

Some newer Intel CPUs have Restricted RSB Alternate (RRSBA) behavior,
which, similar to RSBA described above, also falls back to using the BTB
on RSB underflow. The only difference is that the predicted targets are
restricted to the current domain when eIBRS is enabled:

* “Restricted RSB Alternate (RRSBA) behavior allows alternate branch
  predictors to be used by near RET instructions when the RSB is
  empty. When eIBRS is enabled, the predicted targets of these
  alternate predictors are restricted to those belonging to the
  indirect branch predictor entries of the current prediction domain.
  [[20]](#intel-eibrs-rrsba)

When a CPU with RRSBA is vulnerable to Branch History Injection
[[21]](#bhi-paper) [[22]](#intel-bhi), an RSB underflow could be used for an
intra-mode BTI attack. This is mitigated by clearing the BHB on
kernel entry.

However if the kernel uses retpolines instead of eIBRS, it needs to
disable RRSBA:

* “Where software is using retpoline as a mitigation for BHI or
  intra-mode BTI, and the processor both enumerates RRSBA and
  enumerates RRSBA\_DIS controls, it should disable this behavior.”
  [[23]](#intel-retpoline-rrsba)

---

## References

[[1](#id1)]

[Spectre Returns! Speculation Attacks using the Return Stack Buffer](https://arxiv.org/pdf/1807.07940.pdf)


[[2](#id2)]

“Empty RSB Mitigation on Skylake-generation” in [Retpoline: A Branch Target Injection Mitigation](https://www.intel.com/content/www/us/en/developer/articles/technical/software-security-guidance/technical-documentation/retpoline-branch-target-injection-mitigation.html#inpage-nav-5-1)


[3]
([1](#id3),[2](#id7))

“Mitigation V2-3” in [Software Techniques for Managing Speculation](https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/software-techniques-for-managing-speculation.pdf)


[4]
([1](#id4),[2](#id21))

Whether IBPB is written depends on whether the prev and/or next task is protected from Spectre attacks. It typically requires opting in per task or system-wide. For more details see the documentation for the `spectre_v2_user` cmdline option in [The kernel’s command-line parameters](../kernel-parameters.html).


[[5](#id5)]

IBPB without flushing of branch type predictions. Only exists for AMD.


[[6](#id6)]

“Function 8000\_0008h -- Processor Capacity Parameters and Extended Feature Identification” in [AMD64 Architecture Programmer’s Manual Volume 3: General-Purpose and System Instructions](https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24594.pdf). SBPB behaves the same way according to [this email](https://lore.kernel.org/5175b163a3736ca5fd01cedf406735636c99a).


[[7](#id8)]

[Spectre Attacks: Exploiting Speculative Execution](https://comsec.ethz.ch/wp-content/files/ibpb_sp25.pdf)


[[8](#id9)]

“Introduction” in [Post-barrier Return Stack Buffer Predictions / CVE-2022-26373 / INTEL-SA-00706](https://www.intel.com/content/www/us/en/developer/articles/technical/software-security-guidance/advisory-guidance/post-barrier-return-stack-buffer-predictions.html)


[[9](#id10)]

“Existing Mitigations” in [Technical Guidance for Mitigating Branch Type Confusion](https://www.amd.com/content/dam/amd/en/documents/resources/technical-guidance-for-mitigating-branch-type-confusion.pdf)


[[10](#id11)]

“Enhanced IBRS” in [Indirect Branch Restricted Speculation](https://www.intel.com/content/www/us/en/developer/articles/technical/software-security-guidance/technical-documentation/indirect-branch-restricted-speculation.html)


[[11](#id12)]

“Extended Feature Enable Register (EFER)” in [AMD64 Architecture Programmer’s Manual Volume 2: System Programming](https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf)


[[12](#id13)]

“Enhanced IBRS” in [Indirect Branch Restricted Speculation](https://www.intel.com/content/www/us/en/developer/articles/technical/software-security-guidance/technical-documentation/indirect-branch-restricted-speculation.html)


[[13](#id14)]

[Post-barrier Return Stack Buffer Predictions / CVE-2022-26373 / INTEL-SA-00706](https://www.intel.com/content/www/us/en/developer/articles/technical/software-security-guidance/advisory-guidance/post-barrier-return-stack-buffer-predictions.html)


[14]
([1](#id15),[2](#id19))

[RETBleed: Arbitrary Speculative Code Execution with Return Instruction](https://comsec.ethz.ch/wp-content/files/retbleed_sec22.pdf)


[[15](#id16)]

[Technical Guidance for Mitigating Branch Type Confusion](https://www.amd.com/content/dam/amd/en/documents/resources/technical-guidance-for-mitigating-branch-type-confusion.pdf)


[[16](#id17)]

[Technical Update Regarding Speculative Return Stack Overflow](https://www.amd.com/content/dam/amd/en/documents/corporate/cr/speculative-return-stack-overflow-whitepaper.pdf)


[[17](#id18)]

[Inception: Exposing New Attack Surfaces with Training in Transient Execution](https://comsec.ethz.ch/wp-content/files/inception_sec23.pdf)


[18]
([1](#id20),[2](#id23))

[Return Stack Buffer Underflow / Return Stack Buffer Underflow / CVE-2022-29901, CVE-2022-28693 / INTEL-SA-00702](https://www.intel.com/content/www/us/en/developer/articles/technical/software-security-guidance/advisory-guidance/return-stack-buffer-underflow.html)


[[19](#id22)]

[Indirect Branch Predictor Barrier’](https://www.intel.com/content/www/us/en/developer/articles/technical/software-security-guidance/technical-documentation/indirect-branch-predictor-barrier.html)


[[20](#id24)]

“Guidance for RSBU” in [Return Stack Buffer Underflow / Return Stack Buffer Underflow / CVE-2022-29901, CVE-2022-28693 / INTEL-SA-00702](https://www.intel.com/content/www/us/en/developer/articles/technical/software-security-guidance/advisory-guidance/return-stack-buffer-underflow.html)


[[21](#id25)]

[Branch History Injection: On the Effectiveness of Hardware Mitigations Against Cross-Privilege Spectre-v2 Attacks](http://download.vusec.net/papers/bhi-spectre-bhb_sec22.pdf)


[[22](#id26)]

[Branch History Injection and Intra-mode Branch Target Injection / CVE-2022-0001, CVE-2022-0002 / INTEL-SA-00598](https://www.intel.com/content/www/us/en/developer/articles/technical/software-security-guidance/technical-documentation/branch-history-injection.html)


[[23](#id27)]

“Retpoline” in [Branch History Injection and Intra-mode Branch Target Injection / CVE-2022-0001, CVE-2022-0002 / INTEL-SA-00598](https://www.intel.com/content/www/us/en/developer/articles/technical/software-security-guidance/technical-documentation/branch-history-injection.html)
