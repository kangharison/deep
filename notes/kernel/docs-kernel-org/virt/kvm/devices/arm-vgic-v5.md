# ARM Virtual Generic Interrupt Controller v5 (VGICv5)

> 출처(원문): https://docs.kernel.org/virt/kvm/devices/arm-vgic-v5.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# ARM Virtual Generic Interrupt Controller v5 (VGICv5)

Device types supported:
:   * KVM\_DEV\_TYPE\_ARM\_VGIC\_V5 ARM Generic Interrupt Controller v5.0

Only one VGIC instance may be instantiated through this API. The created VGIC
will act as the VM interrupt controller, requiring emulated user-space devices
to inject interrupts to the VGIC instead of directly to CPUs.

Creating a guest GICv5 device requires a host GICv5 host. The current VGICv5
device only supports PPI interrupts. These can either be injected from emulated
in-kernel devices (such as the Arch Timer, or PMU), or via the KVM\_IRQ\_LINE
ioctl.

Groups:
:   KVM\_DEV\_ARM\_VGIC\_GRP\_CTRL
    :   Attributes:

        > KVM\_DEV\_ARM\_VGIC\_CTRL\_INIT
        > :   request the initialization of the VGIC, no additional parameter in
        >     kvm\_device\_attr.addr. Must be called after all VCPUs have been created.

        KVM\_DEV\_ARM\_VGIC\_USERPSPACE\_PPIs
        :   request the mask of userspace-drivable PPIs. Only a subset of the PPIs can
            be directly driven from userspace with GICv5, and the returned mask
            informs userspace of which it is allowed to drive via KVM\_IRQ\_LINE.

            Userspace must allocate and point to \_\_u64[2] of data in
            kvm\_device\_attr.addr. When this call returns, the provided memory will be
            populated with the userspace PPI mask. The lower \_\_u64 contains the mask
            for the lower 64 PPIS, with the remaining 64 being in the second \_\_u64.

            This is a read-only attribute, and cannot be set. Attempts to set it are
            rejected.

    Errors:

    > |  |  |
    > | --- | --- |
    > | -ENXIO | VGIC not properly configured as required prior to calling this attribute |
    > | -ENODEV | no online VCPU |
    > | -ENOMEM | memory shortage when allocating vgic internal data |
    > | -EFAULT | Invalid guest ram access |
    > | -EBUSY | One or more VCPUS are running |
