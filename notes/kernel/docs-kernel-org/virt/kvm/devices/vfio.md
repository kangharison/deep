# VFIO virtual device

> 출처(원문): https://docs.kernel.org/virt/kvm/devices/vfio.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# VFIO virtual device

Device types supported:

> * KVM\_DEV\_TYPE\_VFIO

Only one VFIO instance may be created per VM. The created device
tracks VFIO files (group or device) in use by the VM and features
of those groups/devices important to the correctness and acceleration
of the VM. As groups/devices are enabled and disabled for use by the
VM, KVM should be updated about their presence. When registered with
KVM, a reference to the VFIO file is held by KVM.

Groups:
:   KVM\_DEV\_VFIO\_FILE
    :   alias: KVM\_DEV\_VFIO\_GROUP

KVM\_DEV\_VFIO\_FILE attributes:
:   KVM\_DEV\_VFIO\_FILE\_ADD: Add a VFIO file (group/device) to VFIO-KVM device
    :   tracking

        kvm\_device\_attr.addr points to an int32\_t file descriptor for the
        VFIO file.

    KVM\_DEV\_VFIO\_FILE\_DEL: Remove a VFIO file (group/device) from VFIO-KVM
    :   device tracking

        kvm\_device\_attr.addr points to an int32\_t file descriptor for the
        VFIO file.

KVM\_DEV\_VFIO\_GROUP (legacy kvm device group restricted to the handling of VFIO group fd):
:   KVM\_DEV\_VFIO\_GROUP\_ADD: same as KVM\_DEV\_VFIO\_FILE\_ADD for group fd only

    KVM\_DEV\_VFIO\_GROUP\_DEL: same as KVM\_DEV\_VFIO\_FILE\_DEL for group fd only

    KVM\_DEV\_VFIO\_GROUP\_SET\_SPAPR\_TCE: attaches a guest visible TCE table
    :   allocated by sPAPR KVM.
        kvm\_device\_attr.addr points to a struct:

        ```
        struct kvm_vfio_spapr_tce {
                __s32   groupfd;
                __s32   tablefd;
        };
        ```

        where:

        * @groupfd is a file descriptor for a VFIO group;
        * @tablefd is a file descriptor for a TCE table allocated via
          KVM\_CREATE\_SPAPR\_TCE.

The FILE/GROUP\_ADD operation above should be invoked prior to accessing the
device file descriptor via VFIO\_GROUP\_GET\_DEVICE\_FD in order to support
drivers which require a kvm pointer to be set in their .`open_device()`
callback. It is the same for device file descriptor via character device
open which gets device access via VFIO\_DEVICE\_BIND\_IOMMUFD. For such file
descriptors, FILE\_ADD should be invoked before VFIO\_DEVICE\_BIND\_IOMMUFD
to support the drivers mentioned in prior sentence as well.
