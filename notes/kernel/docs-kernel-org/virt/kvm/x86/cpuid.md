# KVM CPUID bits

> 출처(원문): https://docs.kernel.org/virt/kvm/x86/cpuid.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# KVM CPUID bits

Author:
:   Glauber Costa <[glommer@gmail.com](mailto:glommer%40gmail.com)>

A guest running on a kvm host, can check some of its features using
cpuid. This is not always guaranteed to work, since userspace can
mask-out some, or even all KVM-related cpuid features before launching
a guest.

KVM cpuid functions are:

function: KVM\_CPUID\_SIGNATURE (0x40000000)

returns:

```
eax = 0x40000001
ebx = 0x4b4d564b
ecx = 0x564b4d56
edx = 0x4d
```

Note that this value in ebx, ecx and edx corresponds to the string “KVMKVMKVM”.
The value in eax corresponds to the maximum cpuid function present in this leaf,
and will be updated if more functions are added in the future.
Note also that old hosts set eax value to 0x0. This should
be interpreted as if the value was 0x40000001.
This function queries the presence of KVM cpuid leafs.

function: define KVM\_CPUID\_FEATURES (0x40000001)

returns:

```
ebx, ecx
eax = an OR'ed group of (1 << flag)
```

where `flag` is defined as below:

| flag | value | meaning |
| --- | --- | --- |
| KVM\_FEATURE\_CLOCKSOURCE | 0 | kvmclock available at msrs 0x11 and 0x12 |
| KVM\_FEATURE\_NOP\_IO\_DELAY | 1 | not necessary to perform delays on PIO operations |
| KVM\_FEATURE\_MMU\_OP | 2 | deprecated |
| KVM\_FEATURE\_CLOCKSOURCE2 | 3 | kvmclock available at msrs 0x4b564d00 and 0x4b564d01 |
| KVM\_FEATURE\_ASYNC\_PF | 4 | async pf can be enabled by writing to msr 0x4b564d02 |
| KVM\_FEATURE\_STEAL\_TIME | 5 | steal time can be enabled by writing to msr 0x4b564d03 |
| KVM\_FEATURE\_PV\_EOI | 6 | paravirtualized end of interrupt handler can be enabled by writing to msr 0x4b564d04 |
| KVM\_FEATURE\_PV\_UNHALT | 7 | guest checks this feature bit before enabling paravirtualized spinlock support |
| KVM\_FEATURE\_PV\_TLB\_FLUSH | 9 | guest checks this feature bit before enabling paravirtualized tlb flush |
| KVM\_FEATURE\_ASYNC\_PF\_VMEXIT | 10 | paravirtualized async PF VM EXIT can be enabled by setting bit 2 when writing to msr 0x4b564d02 |
| KVM\_FEATURE\_PV\_SEND\_IPI | 11 | guest checks this feature bit before enabling paravirtualized send IPIs |
| KVM\_FEATURE\_POLL\_CONTROL | 12 | host-side polling on HLT can be disabled by writing to msr 0x4b564d05. |
| KVM\_FEATURE\_PV\_SCHED\_YIELD | 13 | guest checks this feature bit before using paravirtualized sched yield. |
| KVM\_FEATURE\_ASYNC\_PF\_INT | 14 | guest checks this feature bit before using the second async pf control msr 0x4b564d06 and async pf acknowledgment msr 0x4b564d07. |
| KVM\_FEATURE\_MSI\_EXT\_DEST\_ID | 15 | guest checks this feature bit before using extended destination ID bits in MSI address bits 11-5. |
| KVM\_FEATURE\_HC\_MAP\_GPA\_RANGE | 16 | guest checks this feature bit before using the map gpa range hypercall to notify the page state change |
| KVM\_FEATURE\_MIGRATION\_CONTROL | 17 | guest checks this feature bit before using MSR\_KVM\_MIGRATION\_CONTROL |
| KVM\_FEATURE\_CLOCKSOURCE\_STABLE\_BIT | 24 | host will warn if no guest-side per-cpu warps are expected in kvmclock |

```
edx = an OR'ed group of (1 << flag)
```

Where `flag` here is defined as below:

| flag | value | meaning |
| --- | --- | --- |
| KVM\_HINTS\_REALTIME | 0 | guest checks this feature bit to determine that vCPUs are never preempted for an unlimited time allowing optimizations |
