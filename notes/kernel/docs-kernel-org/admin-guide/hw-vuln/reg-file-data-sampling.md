# Register File Data Sampling (RFDS)

> 출처(원문): https://docs.kernel.org/admin-guide/hw-vuln/reg-file-data-sampling.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Register File Data Sampling (RFDS)

Register File Data Sampling (RFDS) is a microarchitectural vulnerability that
only affects Intel Atom parts(also branded as E-cores). RFDS may allow
a malicious actor to infer data values previously used in floating point
registers, vector registers, or integer registers. RFDS does not provide the
ability to choose which data is inferred. CVE-2023-28746 is assigned to RFDS.

## Affected Processors

Below is the list of affected Intel processors [[1]](#f1):

> | Common name | Family\_Model |
> | --- | --- |
> | ATOM\_GOLDMONT | 06\_5CH |
> | ATOM\_GOLDMONT\_D | 06\_5FH |
> | ATOM\_GOLDMONT\_PLUS | 06\_7AH |
> | ATOM\_TREMONT\_D | 06\_86H |
> | ATOM\_TREMONT | 06\_96H |
> | ALDERLAKE | 06\_97H |
> | ALDERLAKE\_L | 06\_9AH |
> | ATOM\_TREMONT\_L | 06\_9CH |
> | RAPTORLAKE | 06\_B7H |
> | RAPTORLAKE\_P | 06\_BAH |
> | ATOM\_GRACEMONT | 06\_BEH |
> | RAPTORLAKE\_S | 06\_BFH |

## Mitigation

Intel released a microcode update that enables software to clear sensitive
information using the VERW instruction. Like MDS, RFDS deploys the same
mitigation strategy to force the CPU to clear the affected buffers before an
attacker can extract the secrets. This is achieved by using the otherwise
unused and obsolete VERW instruction in combination with a microcode update.
The microcode clears the affected CPU buffers when the VERW instruction is
executed.

### Mitigation points

VERW is executed by the kernel before returning to user space, and by KVM
before VMentry. None of the affected cores support SMT, so VERW is not required
at C-state transitions.

### New bits in IA32\_ARCH\_CAPABILITIES

Newer processors and microcode update on existing affected processors added new
bits to IA32\_ARCH\_CAPABILITIES MSR. These bits can be used to enumerate
vulnerability and mitigation capability:

* Bit 27 - RFDS\_NO - When set, processor is not affected by RFDS.
* Bit 28 - RFDS\_CLEAR - When set, processor is affected by RFDS, and has the
  microcode that clears the affected buffers on VERW execution.

### Mitigation control on the kernel command line

The kernel command line allows to control RFDS mitigation at boot time with the
parameter “reg\_file\_data\_sampling=”. The valid arguments are:

> |  |  |
> | --- | --- |
> | on | If the CPU is vulnerable, enable mitigation; CPU buffer clearing on exit to userspace and before entering a VM. |
> | off | Disables mitigation. |

Mitigation default is selected by CONFIG\_MITIGATION\_RFDS.

### Mitigation status information

The Linux kernel provides a sysfs interface to enumerate the current
vulnerability status of the system: whether the system is vulnerable, and
which mitigations are active. The relevant sysfs file is:

> /sys/devices/system/cpu/vulnerabilities/reg\_file\_data\_sampling

The possible values in this file are:

> |  |  |
> | --- | --- |
> | ‘Not affected’ | The processor is not vulnerable |
> | ‘Vulnerable’ | The processor is vulnerable, but no mitigation enabled |
> | ‘Vulnerable: No microcode’ | The processor is vulnerable but microcode is not updated. |
> | ‘Mitigation: Clear Register File’ | The processor is vulnerable and the CPU buffer clearing mitigation is enabled. |

### References

[[1](#id1)]

Affected Processors
<https://www.intel.com/content/www/us/en/developer/topic-technology/software-security-guidance/processors-affected-consolidated-product-cpu-model.html>
