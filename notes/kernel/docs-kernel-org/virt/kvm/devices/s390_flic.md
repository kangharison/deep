# FLIC (floating interrupt controller)

> 출처(원문): https://docs.kernel.org/virt/kvm/devices/s390_flic.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# FLIC (floating interrupt controller)

FLIC handles floating (non per-cpu) interrupts, i.e. I/O, service and some
machine check interruptions. All interrupts are stored in a per-vm list of
pending interrupts. FLIC performs operations on this list.

Only one FLIC instance may be instantiated.

FLIC provides support to
- add interrupts (KVM\_DEV\_FLIC\_ENQUEUE)
- inspect currently pending interrupts (KVM\_FLIC\_GET\_ALL\_IRQS)
- purge all pending floating interrupts (KVM\_DEV\_FLIC\_CLEAR\_IRQS)
- purge one pending floating I/O interrupt (KVM\_DEV\_FLIC\_CLEAR\_IO\_IRQ)
- enable/disable for the guest transparent async page faults
- register and modify adapter interrupt sources (KVM\_DEV\_FLIC\_ADAPTER\_\*)
- modify AIS (adapter-interruption-suppression) mode state (KVM\_DEV\_FLIC\_AISM)
- inject adapter interrupts on a specified adapter (KVM\_DEV\_FLIC\_AIRQ\_INJECT)
- get/set all AIS mode states (KVM\_DEV\_FLIC\_AISM\_ALL)

Groups:
:   KVM\_DEV\_FLIC\_ENQUEUE
    :   Passes a buffer and length into the kernel which are then injected into
        the list of pending interrupts.
        attr->addr contains the pointer to the buffer and attr->attr contains
        the length of the buffer.
        The format of the data structure kvm\_s390\_irq as it is copied from userspace
        is defined in usr/include/linux/kvm.h.

    KVM\_DEV\_FLIC\_GET\_ALL\_IRQS
    :   Copies all floating interrupts into a buffer provided by userspace.
        When the buffer is too small it returns -ENOMEM, which is the indication
        for userspace to try again with a bigger buffer.

        -ENOBUFS is returned when the allocation of a kernelspace buffer has
        failed.

        -EFAULT is returned when copying data to userspace failed.
        All interrupts remain pending, i.e. are not deleted from the list of
        currently pending interrupts.
        attr->addr contains the userspace address of the buffer into which all
        interrupt data will be copied.
        attr->attr contains the size of the buffer in bytes.

    KVM\_DEV\_FLIC\_CLEAR\_IRQS
    :   Simply deletes all elements from the list of currently pending floating
        interrupts. No interrupts are injected into the guest.

    KVM\_DEV\_FLIC\_CLEAR\_IO\_IRQ
    :   Deletes one (if any) I/O interrupt for a subchannel identified by the
        subsystem identification word passed via the buffer specified by
        attr->addr (address) and attr->attr (length).

    KVM\_DEV\_FLIC\_APF\_ENABLE
    :   Enables async page faults for the guest. So in case of a major page fault
        the host is allowed to handle this async and continues the guest.

        -EINVAL is returned when called on the FLIC of a ucontrol VM.

    KVM\_DEV\_FLIC\_APF\_DISABLE\_WAIT
    :   Disables async page faults for the guest and waits until already pending
        async page faults are done. This is necessary to trigger a completion interrupt
        for every init interrupt before migrating the interrupt list.

        -EINVAL is returned when called on the FLIC of a ucontrol VM.

    KVM\_DEV\_FLIC\_ADAPTER\_REGISTER
    :   > Register an I/O adapter interrupt source. Takes a kvm\_s390\_io\_adapter
        > describing the adapter to register:
        >
        > ```
        > struct kvm_s390_io_adapter {
        >         __u32 id;
        >         __u8 isc;
        >         __u8 maskable;
        >         __u8 swap;
        >         __u8 flags;
        > };
        > ```

        id contains the unique id for the adapter, isc the I/O interruption subclass
        to use, maskable whether this adapter may be masked (interrupts turned off),
        swap whether the indicators need to be byte swapped, and flags contains
        further characteristics of the adapter.

        Currently defined values for ‘flags’ are:

        * KVM\_S390\_ADAPTER\_SUPPRESSIBLE: adapter is subject to AIS
          (adapter-interrupt-suppression) facility. This flag only has an effect if
          the AIS capability is enabled.

        Unknown flag values are ignored.

    KVM\_DEV\_FLIC\_ADAPTER\_MODIFY
    :   Modifies attributes of an existing I/O adapter interrupt source. Takes
        a kvm\_s390\_io\_adapter\_req specifying the adapter and the operation:

        ```
        struct kvm_s390_io_adapter_req {
                __u32 id;
                __u8 type;
                __u8 mask;
                __u16 pad0;
                __u64 addr;
        };
        ```

        id specifies the adapter and type the operation. The supported operations
        are:

        KVM\_S390\_IO\_ADAPTER\_MASK
        :   mask or unmask the adapter, as specified in mask

        KVM\_S390\_IO\_ADAPTER\_MAP
        :   This is now a no-op. The mapping is purely done by the irq route.

        KVM\_S390\_IO\_ADAPTER\_UNMAP
        :   This is now a no-op. The mapping is purely done by the irq route.

    KVM\_DEV\_FLIC\_AISM
    :   modify the adapter-interruption-suppression mode for a given isc if the
        AIS capability is enabled. Takes a kvm\_s390\_ais\_req describing:

        ```
        struct kvm_s390_ais_req {
                __u8 isc;
                __u16 mode;
        };
        ```

        isc contains the target I/O interruption subclass, mode the target
        adapter-interruption-suppression mode. The following modes are
        currently supported:

        * KVM\_S390\_AIS\_MODE\_ALL: ALL-Interruptions Mode, i.e. airq injection
          is always allowed;
        * KVM\_S390\_AIS\_MODE\_SINGLE: SINGLE-Interruption Mode, i.e. airq
          injection is only allowed once and the following adapter interrupts
          will be suppressed until the mode is set again to ALL-Interruptions
          or SINGLE-Interruption mode.

    KVM\_DEV\_FLIC\_AIRQ\_INJECT
    :   Inject adapter interrupts on a specified adapter.
        attr->attr contains the unique id for the adapter, which allows for
        adapter-specific checks and actions.
        For adapters subject to AIS, handle the airq injection suppression for
        an isc according to the adapter-interruption-suppression mode on condition
        that the AIS capability is enabled.

    KVM\_DEV\_FLIC\_AISM\_ALL
    :   Gets or sets the adapter-interruption-suppression mode for all ISCs. Takes
        a kvm\_s390\_ais\_all describing:

        ```
        struct kvm_s390_ais_all {
               __u8 simm; /* Single-Interruption-Mode mask */
               __u8 nimm; /* No-Interruption-Mode mask *
        };
        ```

        simm contains Single-Interruption-Mode mask for all ISCs, nimm contains
        No-Interruption-Mode mask for all ISCs. Each bit in simm and nimm corresponds
        to an ISC (MSB0 bit 0 to ISC 0 and so on). The combination of simm bit and
        nimm bit presents AIS mode for a ISC.

        KVM\_DEV\_FLIC\_AISM\_ALL is indicated by KVM\_CAP\_S390\_AIS\_MIGRATION.

Note: The KVM\_SET\_DEVICE\_ATTR/KVM\_GET\_DEVICE\_ATTR device ioctls executed on
FLIC with an unknown group or attribute gives the error code EINVAL (instead of
ENXIO, as specified in the API documentation). It is not possible to conclude
that a FLIC operation is unavailable based on the error code resulting from a
usage attempt.

Note

The KVM\_DEV\_FLIC\_CLEAR\_IO\_IRQ ioctl will return EINVAL in case a
zero schid is specified.
