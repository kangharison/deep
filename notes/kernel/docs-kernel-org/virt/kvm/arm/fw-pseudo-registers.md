# ARM firmware pseudo-registers interface

> 출처(원문): https://docs.kernel.org/virt/kvm/arm/fw-pseudo-registers.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# ARM firmware pseudo-registers interface

KVM handles the hypercall services as requested by the guests. New hypercall
services are regularly made available by the ARM specification or by KVM (as
vendor services) if they make sense from a virtualization point of view.

This means that a guest booted on two different versions of KVM can observe
two different “firmware” revisions. This could cause issues if a given guest
is tied to a particular version of a hypercall service, or if a migration
causes a different version to be exposed out of the blue to an unsuspecting
guest.

In order to remedy this situation, KVM exposes a set of “firmware
pseudo-registers” that can be manipulated using the GET/SET\_ONE\_REG
interface. These registers can be saved/restored by userspace, and set
to a convenient value as required.

The following registers are defined:

* KVM\_REG\_ARM\_PSCI\_VERSION:

  KVM implements the PSCI (Power State Coordination Interface)
  specification in order to provide services such as CPU on/off, reset
  and power-off to the guest.

  + Only valid if the vcpu has the KVM\_ARM\_VCPU\_PSCI\_0\_2 feature set
    (and thus has already been initialized)
  + Returns the current PSCI version on GET\_ONE\_REG (defaulting to the
    highest PSCI version implemented by KVM and compatible with v0.2)
  + Allows any PSCI version implemented by KVM and compatible with
    v0.2 to be set with SET\_ONE\_REG
  + Affects the whole VM (even if the register view is per-vcpu)
* KVM\_REG\_ARM\_SMCCC\_ARCH\_WORKAROUND\_1:
  :   Holds the state of the firmware support to mitigate CVE-2017-5715, as
      offered by KVM to the guest via a HVC call. The workaround is described
      under SMCCC\_ARCH\_WORKAROUND\_1 in [1].

  Accepted values are:

  > KVM\_REG\_ARM\_SMCCC\_ARCH\_WORKAROUND\_1\_NOT\_AVAIL:
  > :   KVM does not offer
  >     firmware support for the workaround. The mitigation status for the
  >     guest is unknown.
  >
  > KVM\_REG\_ARM\_SMCCC\_ARCH\_WORKAROUND\_1\_AVAIL:
  > :   The workaround HVC call is
  >     available to the guest and required for the mitigation.
  >
  > KVM\_REG\_ARM\_SMCCC\_ARCH\_WORKAROUND\_1\_NOT\_REQUIRED:
  > :   The workaround HVC call
  >     is available to the guest, but it is not needed on this VCPU.
* KVM\_REG\_ARM\_SMCCC\_ARCH\_WORKAROUND\_2:
  :   Holds the state of the firmware support to mitigate CVE-2018-3639, as
      offered by KVM to the guest via a HVC call. The workaround is described
      under SMCCC\_ARCH\_WORKAROUND\_2 in [[1]](#id2).

  Accepted values are:

  > KVM\_REG\_ARM\_SMCCC\_ARCH\_WORKAROUND\_2\_NOT\_AVAIL:
  > :   A workaround is not
  >     available. KVM does not offer firmware support for the workaround.
  >
  > KVM\_REG\_ARM\_SMCCC\_ARCH\_WORKAROUND\_2\_UNKNOWN:
  > :   The workaround state is
  >     unknown. KVM does not offer firmware support for the workaround.
  >
  > KVM\_REG\_ARM\_SMCCC\_ARCH\_WORKAROUND\_2\_AVAIL:
  > :   The workaround is available,
  >     and can be disabled by a vCPU. If
  >     KVM\_REG\_ARM\_SMCCC\_ARCH\_WORKAROUND\_2\_ENABLED is set, it is active for
  >     this vCPU.
  >
  > KVM\_REG\_ARM\_SMCCC\_ARCH\_WORKAROUND\_2\_NOT\_REQUIRED:
  > :   The workaround is always active on this vCPU or it is not needed.

## Bitmap Feature Firmware Registers

Contrary to the above registers, the following registers exposes the
hypercall services in the form of a feature-bitmap to the userspace. This
bitmap is translated to the services that are available to the guest.
There is a register defined per service call owner and can be accessed via
GET/SET\_ONE\_REG interface.

By default, these registers are set with the upper limit of the features
that are supported. This way userspace can discover all the usable
hypercall services via GET\_ONE\_REG. The user-space can write-back the
desired bitmap back via SET\_ONE\_REG. The features for the registers that
are untouched, probably because userspace isn’t aware of them, will be
exposed as is to the guest.

Note that KVM will not allow the userspace to configure the registers
anymore once any of the vCPUs has run at least once. Instead, it will
return a -EBUSY.

The pseudo-firmware bitmap register are as follows:

* KVM\_REG\_ARM\_STD\_BMAP:
  :   Controls the bitmap of the ARM Standard Secure Service Calls.

  The following bits are accepted:

  > Bit-0: KVM\_REG\_ARM\_STD\_BIT\_TRNG\_V1\_0:
  > :   The bit represents the services offered under v1.0 of ARM True Random
  >     Number Generator (TRNG) specification, ARM DEN0098.
* KVM\_REG\_ARM\_STD\_HYP\_BMAP:
  :   Controls the bitmap of the ARM Standard Hypervisor Service Calls.

  The following bits are accepted:

  > Bit-0: KVM\_REG\_ARM\_STD\_HYP\_BIT\_PV\_TIME:
  > :   The bit represents the Paravirtualized Time service as represented by
  >     ARM DEN0057A.
* KVM\_REG\_ARM\_VENDOR\_HYP\_BMAP:
  :   Controls the bitmap of the Vendor specific Hypervisor Service Calls[0-63].

  The following bits are accepted:

  > Bit-0: KVM\_REG\_ARM\_VENDOR\_HYP\_BIT\_FUNC\_FEAT
  > :   The bit represents the ARM\_SMCCC\_VENDOR\_HYP\_KVM\_FEATURES\_FUNC\_ID
  >     and ARM\_SMCCC\_VENDOR\_HYP\_CALL\_UID\_FUNC\_ID function-ids.
  >
  > Bit-1: KVM\_REG\_ARM\_VENDOR\_HYP\_BIT\_PTP:
  > :   The bit represents the Precision Time Protocol KVM service.
* KVM\_REG\_ARM\_VENDOR\_HYP\_BMAP\_2:
  :   Controls the bitmap of the Vendor specific Hypervisor Service Calls[64-127].

  The following bits are accepted:

  > Bit-0: KVM\_REG\_ARM\_VENDOR\_HYP\_BIT\_DISCOVER\_IMPL\_VER
  > :   This represents the ARM\_SMCCC\_VENDOR\_HYP\_KVM\_DISCOVER\_IMPL\_VER\_FUNC\_ID
  >     function-id. This is reset to 0.
  >
  > Bit-1: KVM\_REG\_ARM\_VENDOR\_HYP\_BIT\_DISCOVER\_IMPL\_CPUS
  > :   This represents the ARM\_SMCCC\_VENDOR\_HYP\_KVM\_DISCOVER\_IMPL\_CPUS\_FUNC\_ID
  >     function-id. This is reset to 0.

Errors:

> |  |  |
> | --- | --- |
> | -ENOENT | Unknown register accessed. |
> | -EBUSY | Attempt a ‘write’ to the register after the VM has started. |
> | -EINVAL | Invalid bitmap written to the register. |

[[1](#id1)]

<https://developer.arm.com/-/media/developer/pdf/ARM_DEN_0070A_Firmware_interfaces_for_mitigating_CVE-2017-5715.pdf>
