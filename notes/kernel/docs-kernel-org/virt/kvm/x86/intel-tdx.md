# Intel Trust Domain Extensions (TDX)

> 출처(원문): https://docs.kernel.org/virt/kvm/x86/intel-tdx.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Intel Trust Domain Extensions (TDX)

## Overview

Intel’s Trust Domain Extensions (TDX) protect confidential guest VMs from the
host and physical attacks. A CPU-attested software module called ‘the TDX
module’ runs inside a new CPU isolated range to provide the functionalities to
manage and run protected VMs, a.k.a, TDX guests or TDs.

Please refer to [1] for the whitepaper, specifications and other resources.

This documentation describes TDX-specific KVM ABIs. The TDX module needs to be
initialized before it can be used by KVM to run any TDX guests. The host
core-kernel provides the support of initializing the TDX module, which is
described in the [Intel Trust Domain Extensions (TDX)](../../../arch/x86/tdx.html).

## API description

### KVM\_MEMORY\_ENCRYPT\_OP

Type:
:   vm ioctl, vcpu ioctl

For TDX operations, KVM\_MEMORY\_ENCRYPT\_OP is re-purposed to be generic
ioctl with TDX specific sub-ioctl() commands.

```
/* Trust Domain Extensions sub-ioctl() commands. */
enum kvm_tdx_cmd_id {
        KVM_TDX_CAPABILITIES = 0,
        KVM_TDX_INIT_VM,
        KVM_TDX_INIT_VCPU,
        KVM_TDX_INIT_MEM_REGION,
        KVM_TDX_FINALIZE_VM,
        KVM_TDX_GET_CPUID,

        KVM_TDX_CMD_NR_MAX,
};

struct kvm_tdx_cmd {
      /* enum kvm_tdx_cmd_id */
      __u32 id;
      /* flags for sub-command. If sub-command doesn't use this, set zero. */
      __u32 flags;
      /*
       * data for each sub-command. An immediate or a pointer to the actual
       * data in process virtual address.  If sub-command doesn't use it,
       * set zero.
       */
      __u64 data;
      /*
       * Auxiliary error code.  The sub-command may return TDX SEAMCALL
       * status code in addition to -Exxx.
       */
      __u64 hw_error;
};
```

### KVM\_TDX\_CAPABILITIES

Type:
:   vm ioctl

Returns:
:   0 on success, <0 on error

Return the TDX capabilities that current KVM supports with the specific TDX
module loaded in the system. It reports what features/capabilities are allowed
to be configured to the TDX guest.

* id: KVM\_TDX\_CAPABILITIES
* flags: must be 0
* data: pointer to `struct kvm_tdx_capabilities`
* hw\_error: must be 0

```
struct kvm_tdx_capabilities {
      __u64 supported_attrs;
      __u64 supported_xfam;

      /* TDG.VP.VMCALL hypercalls executed in kernel and forwarded to
       * userspace, respectively
       */
      __u64 kernel_tdvmcallinfo_1_r11;
      __u64 user_tdvmcallinfo_1_r11;

      /* TDG.VP.VMCALL instruction executions subfunctions executed in kernel
       * and forwarded to userspace, respectively
       */
      __u64 kernel_tdvmcallinfo_1_r12;
      __u64 user_tdvmcallinfo_1_r12;

      __u64 reserved[250];

      /* Configurable CPUID bits for userspace */
      struct kvm_cpuid2 cpuid;
};
```

### KVM\_TDX\_INIT\_VM

Type:
:   vm ioctl

Returns:
:   0 on success, <0 on error

Perform TDX specific VM initialization. This needs to be called after
KVM\_CREATE\_VM and before creating any VCPUs.

* id: KVM\_TDX\_INIT\_VM
* flags: must be 0
* data: pointer to `struct kvm_tdx_init_vm`
* hw\_error: must be 0

```
struct kvm_tdx_init_vm {
        __u64 attributes;
        __u64 xfam;
        __u64 mrconfigid[6];          /* sha384 digest */
        __u64 mrowner[6];             /* sha384 digest */
        __u64 mrownerconfig[6];       /* sha384 digest */

        /* The total space for TD_PARAMS before the CPUIDs is 256 bytes */
        __u64 reserved[12];

      /*
       * Call KVM_TDX_INIT_VM before vcpu creation, thus before
       * KVM_SET_CPUID2.
       * This configuration supersedes KVM_SET_CPUID2s for VCPUs because the
       * TDX module directly virtualizes those CPUIDs without VMM.  The user
       * space VMM, e.g. qemu, should make KVM_SET_CPUID2 consistent with
       * those values.  If it doesn't, KVM may have wrong idea of vCPUIDs of
       * the guest, and KVM may wrongly emulate CPUIDs or MSRs that the TDX
       * module doesn't virtualize.
       */
        struct kvm_cpuid2 cpuid;
};
```

### KVM\_TDX\_INIT\_VCPU

Type:
:   vcpu ioctl

Returns:
:   0 on success, <0 on error

Perform TDX specific VCPU initialization.

* id: KVM\_TDX\_INIT\_VCPU
* flags: must be 0
* data: initial value of the guest TD VCPU RCX
* hw\_error: must be 0

### KVM\_TDX\_INIT\_MEM\_REGION

Type:
:   vcpu ioctl

Returns:
:   0 on success, <0 on error

Initialize @nr\_pages TDX guest private memory starting from @gpa with userspace
provided data from @source\_addr. @source\_addr must be PAGE\_SIZE-aligned.

Note, before calling this sub command, memory attribute of the range
[gpa, gpa + nr\_pages] needs to be private. Userspace can use
KVM\_SET\_MEMORY\_ATTRIBUTES to set the attribute.

If KVM\_TDX\_MEASURE\_MEMORY\_REGION flag is specified, it also extends measurement.

* id: KVM\_TDX\_INIT\_MEM\_REGION
* flags: currently only KVM\_TDX\_MEASURE\_MEMORY\_REGION is defined
* data: pointer to `struct kvm_tdx_init_mem_region`
* hw\_error: must be 0

```
#define KVM_TDX_MEASURE_MEMORY_REGION   (1UL << 0)

struct kvm_tdx_init_mem_region {
        __u64 source_addr;
        __u64 gpa;
        __u64 nr_pages;
};
```

### KVM\_TDX\_FINALIZE\_VM

Type:
:   vm ioctl

Returns:
:   0 on success, <0 on error

Complete measurement of the initial TD contents and mark it ready to run.

* id: KVM\_TDX\_FINALIZE\_VM
* flags: must be 0
* data: must be 0
* hw\_error: must be 0

### KVM\_TDX\_GET\_CPUID

Type:
:   vcpu ioctl

Returns:
:   0 on success, <0 on error

Get the CPUID values that the TDX module virtualizes for the TD guest.
When it returns -E2BIG, the user space should allocate a larger buffer and
retry. The minimum buffer size is updated in the nent field of the
`struct kvm_cpuid2`.

* id: KVM\_TDX\_GET\_CPUID
* flags: must be 0
* data: pointer to `struct kvm_cpuid2` (in/out)
* hw\_error: must be 0 (out)

```
struct kvm_cpuid2 {
        __u32 nent;
        __u32 padding;
        struct kvm_cpuid_entry2 entries[0];
};

struct kvm_cpuid_entry2 {
        __u32 function;
        __u32 index;
        __u32 flags;
        __u32 eax;
        __u32 ebx;
        __u32 ecx;
        __u32 edx;
        __u32 padding[3];
};
```

## KVM TDX creation flow

In addition to the standard KVM flow, new TDX ioctls need to be called. The
control flow is as follows:

1. Check system wide capability

   * KVM\_CAP\_VM\_TYPES: Check if VM type is supported and if KVM\_X86\_TDX\_VM
     is supported.
2. Create VM

   * KVM\_CREATE\_VM
   * KVM\_TDX\_CAPABILITIES: Query TDX capabilities for creating TDX guests.
   * KVM\_CHECK\_EXTENSION(KVM\_CAP\_MAX\_VCPUS): Query maximum VCPUs the TD can
     support at VM level (TDX has its own limitation on this).
   * KVM\_SET\_TSC\_KHZ: Configure TD’s TSC frequency if a different TSC frequency
     than host is desired. This is Optional.
   * KVM\_TDX\_INIT\_VM: Pass TDX specific VM parameters.
3. Create VCPU

   * KVM\_CREATE\_VCPU
   * KVM\_TDX\_INIT\_VCPU: Pass TDX specific VCPU parameters.
   * KVM\_SET\_CPUID2: Configure TD’s CPUIDs.
   * KVM\_SET\_MSRS: Configure TD’s MSRs.
4. Initialize initial guest memory

   * Prepare content of initial guest memory.
   * KVM\_TDX\_INIT\_MEM\_REGION: Add initial guest memory.
   * KVM\_TDX\_FINALIZE\_VM: Finalize the measurement of the TDX guest.
5. Run VCPU

## References

<https://www.intel.com/content/www/us/en/developer/tools/trust-domain-extensions/documentation.html>
