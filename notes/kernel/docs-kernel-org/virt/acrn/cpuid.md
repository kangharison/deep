# ACRN CPUID bits

> 출처(원문): https://docs.kernel.org/virt/acrn/cpuid.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# ACRN CPUID bits

A guest VM running on an ACRN hypervisor can check some of its features using
CPUID.

ACRN cpuid functions are:

function: 0x40000000

returns:

```
eax = 0x40000010
ebx = 0x4e524341
ecx = 0x4e524341
edx = 0x4e524341
```

Note that this value in ebx, ecx and edx corresponds to the string
“ACRNACRNACRN”. The value in eax corresponds to the maximum cpuid function
present in this leaf, and will be updated if more functions are added in the
future.

function: define ACRN\_CPUID\_FEATURES (0x40000001)

returns:

```
ebx, ecx, edx
eax = an OR'ed group of (1 << flag)
```

where `flag` is defined as below:

| flag | value | meaning |
| --- | --- | --- |
| ACRN\_FEATURE\_PRIVILEGED\_VM | 0 | guest VM is a privileged VM |

function: 0x40000010

returns:

```
ebx, ecx, edx
eax = (Virtual) TSC frequency in kHz.
```
