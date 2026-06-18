# Generic vcpu interface

> 출처(원문): https://docs.kernel.org/virt/kvm/devices/vcpu.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Generic vcpu interface

The virtual cpu “device” also accepts the ioctls KVM\_SET\_DEVICE\_ATTR,
KVM\_GET\_DEVICE\_ATTR, and KVM\_HAS\_DEVICE\_ATTR. The interface uses the same `struct
kvm_device_attr` as other devices, but targets VCPU-wide settings and controls.

The groups and attributes per virtual cpu, if any, are architecture specific.

## 1. GROUP: KVM\_ARM\_VCPU\_PMU\_V3\_CTRL

Architectures:
:   ARM64

### 1.1. ATTRIBUTE: KVM\_ARM\_VCPU\_PMU\_V3\_IRQ

Parameters:
:   in kvm\_device\_attr.addr the address for PMU overflow interrupt is a
    pointer to an int

Returns:

> |  |  |
> | --- | --- |
> | -EBUSY | The PMU overflow interrupt is already set |
> | -EFAULT | Error reading interrupt number |
> | -ENXIO | PMUv3 not supported or the overflow interrupt not set when attempting to get it |
> | -ENODEV | KVM\_ARM\_VCPU\_PMU\_V3 feature missing from VCPU |
> | -EINVAL | Invalid PMU overflow interrupt number supplied or trying to set the IRQ number without using an in-kernel irqchip. |

A value describing the PMUv3 (Performance Monitor Unit v3) overflow interrupt
number for this vcpu. This interrupt could be a PPI or SPI, but the interrupt
type must be same for each vcpu. As a PPI, the interrupt number is the same for
all vcpus, while as an SPI it must be a separate number per vcpu. For
GICv5-based guests, the architected PPI (23) must be used.

### 1.2 ATTRIBUTE: KVM\_ARM\_VCPU\_PMU\_V3\_INIT

Parameters:
:   no additional parameter in kvm\_device\_attr.addr

Returns:

> |  |  |
> | --- | --- |
> | -EEXIST | Interrupt number already used |
> | -ENODEV | PMUv3 not supported or GIC not initialized |
> | -ENXIO | PMUv3 not supported, missing VCPU feature or interrupt number not set (non-GICv5 guests, only) |
> | -EBUSY | PMUv3 already initialized |

Request the initialization of the PMUv3. If using the PMUv3 with an in-kernel
virtual GIC implementation, this must be done after initializing the in-kernel
irqchip.

### 1.3 ATTRIBUTE: KVM\_ARM\_VCPU\_PMU\_V3\_FILTER

Parameters:
:   in kvm\_device\_attr.addr the address for a PMU event filter is a
    pointer to a `struct kvm_pmu_event_filter`

Returns:
:   |  |  |
    | --- | --- |
    | -ENODEV | PMUv3 not supported or GIC not initialized |
    | -ENXIO | PMUv3 not properly configured or in-kernel irqchip not configured as required prior to calling this attribute |
    | -EBUSY | PMUv3 already initialized or a VCPU has already run |
    | -EINVAL | Invalid filter range |

Request the installation of a PMU event filter described as follows:

```
struct kvm_pmu_event_filter {
        __u16       base_event;
        __u16       nevents;

#define KVM_PMU_EVENT_ALLOW 0
#define KVM_PMU_EVENT_DENY  1

        __u8        action;
        __u8        pad[3];
};
```

A filter range is defined as the range [@base\_event, @base\_event + @nevents),
together with an @action (KVM\_PMU\_EVENT\_ALLOW or KVM\_PMU\_EVENT\_DENY). The
first registered range defines the global policy (global ALLOW if the first
@action is DENY, global DENY if the first @action is ALLOW). Multiple ranges
can be programmed, and must fit within the event space defined by the PMU
architecture (10 bits on ARMv8.0, 16 bits from ARMv8.1 onwards).

Note: “Cancelling” a filter by registering the opposite action for the same
range doesn’t change the default action. For example, installing an ALLOW
filter for event range [0:10) as the first filter and then applying a DENY
action for the same range will leave the whole range as disabled.

Restrictions: Event 0 (SW\_INCR) is never filtered, as it doesn’t count a
hardware event. Filtering event 0x1E (CHAIN) has no effect either, as it
isn’t strictly speaking an event. Filtering the cycle counter is possible
using event 0x11 (CPU\_CYCLES).

### 1.4 ATTRIBUTE: KVM\_ARM\_VCPU\_PMU\_V3\_SET\_PMU

Parameters:
:   in kvm\_device\_attr.addr the address to an int representing the PMU
    identifier.

Returns:
:   |  |  |
    | --- | --- |
    | -EBUSY | PMUv3 already initialized, a VCPU has already run or an event filter has already been set |
    | -EFAULT | Error accessing the PMU identifier |
    | -ENXIO | PMU not found |
    | -ENODEV | PMUv3 not supported or GIC not initialized |
    | -ENOMEM | Could not allocate memory |

Request that the VCPU uses the specified hardware PMU when creating guest events
for the purpose of PMU emulation. The PMU identifier can be read from the “type”
file for the desired PMU instance under /sys/devices (or, equivalent,
/sys/bus/even\_source). This attribute is particularly useful on heterogeneous
systems where there are at least two CPU PMUs on the system. The PMU that is set
for one VCPU will be used by all the other VCPUs. It isn’t possible to set a PMU
if a PMU event filter is already present.

Note that KVM will not make any attempts to run the VCPU on the physical CPUs
associated with the PMU specified by this attribute. This is entirely left to
userspace. However, attempting to run the VCPU on a physical CPU not supported
by the PMU will fail and KVM\_RUN will return with
exit\_reason = KVM\_EXIT\_FAIL\_ENTRY and populate the fail\_entry `struct by` setting
hardare\_entry\_failure\_reason field to KVM\_EXIT\_FAIL\_ENTRY\_CPU\_UNSUPPORTED and
the cpu field to the processor id.

### 1.5 ATTRIBUTE: KVM\_ARM\_VCPU\_PMU\_V3\_SET\_NR\_COUNTERS

Parameters:
:   in kvm\_device\_attr.addr the address to an unsigned int
    representing the maximum value taken by PMCR\_EL0.N

Returns:
:   |  |  |
    | --- | --- |
    | -EBUSY | PMUv3 already initialized, a VCPU has already run or an event filter has already been set |
    | -EFAULT | Error accessing the value pointed to by addr |
    | -ENODEV | PMUv3 not supported or GIC not initialized |
    | -EINVAL | No PMUv3 explicitly selected, or value of N out of range |

Set the number of implemented event counters in the virtual PMU. This
mandates that a PMU has explicitly been selected via
KVM\_ARM\_VCPU\_PMU\_V3\_SET\_PMU, and will fail when no PMU has been
explicitly selected, or the number of counters is out of range for the
selected PMU. Selecting a new PMU cancels the effect of setting this
attribute.

## 2. GROUP: KVM\_ARM\_VCPU\_TIMER\_CTRL

Architectures:
:   ARM64

### 2.1. ATTRIBUTES: KVM\_ARM\_VCPU\_TIMER\_IRQ\_{VTIMER,PTIMER,HVTIMER,HPTIMER}

Parameters:
:   in kvm\_device\_attr.addr the address for the timer interrupt is a
    pointer to an int

Returns:

> |  |  |
> | --- | --- |
> | -EINVAL | Invalid timer interrupt number |
> | -EBUSY | One or more VCPUs has already run |

A value describing the architected timer interrupt number when connected to an
in-kernel virtual GIC. These must be a PPI (16 <= intid < 32). Setting the
attribute overrides the default values (see below).

|  |  |
| --- | --- |
| KVM\_ARM\_VCPU\_TIMER\_IRQ\_VTIMER | The EL1 virtual timer intid (default: 27) |
| KVM\_ARM\_VCPU\_TIMER\_IRQ\_PTIMER | The EL1 physical timer intid (default: 30) |
| KVM\_ARM\_VCPU\_TIMER\_IRQ\_HVTIMER | The EL2 virtual timer intid (default: 28) |
| KVM\_ARM\_VCPU\_TIMER\_IRQ\_HPTIMER | The EL2 physical timer intid (default: 26) |

Setting the same PPI for different timers will prevent the VCPUs from running.
Setting the interrupt number on a VCPU configures all VCPUs created at that
time to use the number provided for a given timer, overwriting any previously
configured values on other VCPUs. Userspace should configure the interrupt
numbers on at least one VCPU after creating all VCPUs and before running any
VCPUs.

## 3. GROUP: KVM\_ARM\_VCPU\_PVTIME\_CTRL

Architectures:
:   ARM64

### 3.1 ATTRIBUTE: KVM\_ARM\_VCPU\_PVTIME\_IPA

Parameters:
:   64-bit base address

Returns:

> |  |  |
> | --- | --- |
> | -ENXIO | Stolen time not implemented |
> | -EEXIST | Base address already set for this VCPU |
> | -EINVAL | Base address not 64 byte aligned |

Specifies the base address of the stolen time structure for this VCPU. The
base address must be 64 byte aligned and exist within a valid guest memory
region. See [Paravirtualized time support for arm64](../arm/pvtime.html) for more information
including the layout of the stolen time structure.

## 4. GROUP: KVM\_VCPU\_TSC\_CTRL

Architectures:
:   x86

4.1 ATTRIBUTE: KVM\_VCPU\_TSC\_OFFSET

Parameters:
:   64-bit unsigned TSC offset

Returns:

> |  |  |
> | --- | --- |
> | -EFAULT | Error reading/writing the provided parameter address. |
> | -ENXIO | Attribute not supported |

Specifies the guest’s TSC offset relative to the host’s TSC. The guest’s
TSC is then derived by the following equation:

> guest\_tsc = host\_tsc + KVM\_VCPU\_TSC\_OFFSET

This attribute is useful to adjust the guest’s TSC on live migration,
so that the TSC counts the time during which the VM was paused. The
following describes a possible algorithm to use for this purpose.

From the source VMM process:

1. Invoke the KVM\_GET\_CLOCK ioctl to record the host TSC (tsc\_src),
   kvmclock nanoseconds (guest\_src), and host CLOCK\_REALTIME nanoseconds
   (host\_src).
2. Read the KVM\_VCPU\_TSC\_OFFSET attribute for every vCPU to record the
   guest TSC offset (ofs\_src[i]).
3. Invoke the KVM\_GET\_TSC\_KHZ ioctl to record the frequency of the
   guest’s TSC (freq).

From the destination VMM process:

4. Invoke the KVM\_SET\_CLOCK ioctl, providing the source nanoseconds from
   kvmclock (guest\_src) and CLOCK\_REALTIME (host\_src) in their respective
   fields. Ensure that the KVM\_CLOCK\_REALTIME flag is set in the provided
   structure.

   KVM will advance the VM’s kvmclock to account for elapsed time since
   recording the clock values. Note that this will cause problems in
   the guest (e.g., timeouts) unless CLOCK\_REALTIME is synchronized
   between the source and destination, and a reasonably short time passes
   between the source pausing the VMs and the destination executing
   steps 4-7.
5. Invoke the KVM\_GET\_CLOCK ioctl to record the host TSC (tsc\_dest) and
   kvmclock nanoseconds (guest\_dest).
6. Adjust the guest TSC offsets for every vCPU to account for (1) time
   elapsed since recording state and (2) difference in TSCs between the
   source and destination machine:

   ofs\_dst[i] = ofs\_src[i] -
   :   (guest\_src - guest\_dest) \* freq +
       (tsc\_src - tsc\_dest)

   (“ofs[i] + tsc - guest \* freq” is the guest TSC value corresponding to
   a time of 0 in kvmclock. The above formula ensures that it is the
   same on the destination as it was on the source).
7. Write the KVM\_VCPU\_TSC\_OFFSET attribute for every vCPU with the
   respective value derived in the previous step.
