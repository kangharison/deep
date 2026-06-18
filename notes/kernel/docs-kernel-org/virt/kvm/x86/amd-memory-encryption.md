# Secure Encrypted Virtualization (SEV)

> 출처(원문): https://docs.kernel.org/virt/kvm/x86/amd-memory-encryption.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Secure Encrypted Virtualization (SEV)

## Overview

Secure Encrypted Virtualization (SEV) is a feature found on AMD processors.

SEV is an extension to the AMD-V architecture which supports running
virtual machines (VMs) under the control of a hypervisor. When enabled,
the memory contents of a VM will be transparently encrypted with a key
unique to that VM.

The hypervisor can determine the SEV support through the CPUID
instruction. The CPUID function 0x8000001f reports information related
to SEV:

```
0x8000001f[eax]:
                Bit[1]  indicates support for SEV
    ...
          [ecx]:
                Bits[31:0]  Number of encrypted guests supported simultaneously
```

If support for SEV is present, MSR 0xc001\_0010 (MSR\_AMD64\_SYSCFG) and MSR 0xc001\_0015
(MSR\_K7\_HWCR) can be used to determine if it can be enabled:

```
0xc001_0010:
        Bit[23]    1 = memory encryption can be enabled
                   0 = memory encryption can not be enabled

0xc001_0015:
        Bit[0]     1 = memory encryption can be enabled
                   0 = memory encryption can not be enabled
```

When SEV support is available, it can be enabled in a specific VM by
setting the SEV bit before executing VMRUN.:

```
VMCB[0x90]:
        Bit[1]      1 = SEV is enabled
                    0 = SEV is disabled
```

SEV hardware uses ASIDs to associate a memory encryption key with a VM.
Hence, the ASID for the SEV-enabled guests must be from 1 to a maximum value
defined in the CPUID 0x8000001f[ecx] field.

## The KVM\_MEMORY\_ENCRYPT\_OP ioctl

The main ioctl to access SEV is KVM\_MEMORY\_ENCRYPT\_OP, which operates on
the VM file descriptor. If the argument to KVM\_MEMORY\_ENCRYPT\_OP is NULL,
the ioctl returns 0 if SEV is enabled and `ENOTTY` if it is disabled
(on some older versions of Linux, the ioctl tries to run normally even
with a NULL argument, and therefore will likely return `EFAULT` instead
of zero if SEV is enabled). If non-NULL, the argument to
KVM\_MEMORY\_ENCRYPT\_OP must be a `struct kvm_sev_cmd`:

```
struct kvm_sev_cmd {
        __u32 id;
        __u64 data;
        __u32 error;
        __u32 sev_fd;
};
```

The `id` field contains the subcommand, and the `data` field points to
another `struct containing` arguments specific to command. The `sev_fd`
should point to a file descriptor that is opened on the `/dev/sev`
device, if needed (see individual commands).

On output, `error` is zero on success, or an error code. Error codes
are defined in `<linux/psp-dev.h>`.

KVM implements the following commands to support common lifecycle events of SEV
guests, such as launching, running, snapshotting, migrating and decommissioning.

### 1. KVM\_SEV\_INIT2

The KVM\_SEV\_INIT2 command is used by the hypervisor to initialize the SEV platform
context. In a typical workflow, this command should be the first command issued.

For this command to be accepted, either KVM\_X86\_SEV\_VM or KVM\_X86\_SEV\_ES\_VM
must have been passed to the KVM\_CREATE\_VM ioctl. A virtual machine created
with those machine types in turn cannot be run until KVM\_SEV\_INIT2 is invoked.

Parameters: `struct kvm_sev_init` (in)

Returns: 0 on success, -negative on error

```
struct kvm_sev_init {
        __u64 vmsa_features;  /* initial value of features field in VMSA */
        __u32 flags;          /* must be 0 */
        __u16 ghcb_version;   /* maximum guest GHCB version allowed */
        __u16 pad1;
        __u32 pad2[8];
};
```

It is an error if the hypervisor does not support any of the bits that
are set in `flags` or `vmsa_features`. `vmsa_features` must be
0 for SEV virtual machines, as they do not have a VMSA.

`ghcb_version` must be 0 for SEV virtual machines, as they do not issue GHCB
requests. If `ghcb_version` is 0 for any other guest type, then the maximum
allowed guest GHCB protocol will default to version 2.

This command replaces the deprecated KVM\_SEV\_INIT and KVM\_SEV\_ES\_INIT commands.
The commands did not have any parameters (the `` `data` `` field was unused) and
only work for the KVM\_X86\_DEFAULT\_VM machine type (0).

They behave as if:

* the VM type is KVM\_X86\_SEV\_VM for KVM\_SEV\_INIT, or KVM\_X86\_SEV\_ES\_VM for
  KVM\_SEV\_ES\_INIT
* the `flags` and `vmsa_features` fields of `struct kvm_sev_init` are
  set to zero, and `ghcb_version` is set to 0 for KVM\_SEV\_INIT and 1 for
  KVM\_SEV\_ES\_INIT.

If the `KVM_X86_SEV_VMSA_FEATURES` attribute does not exist, the hypervisor only
supports KVM\_SEV\_INIT and KVM\_SEV\_ES\_INIT. In that case, note that KVM\_SEV\_ES\_INIT
might set the debug swap VMSA feature (bit 5) depending on the value of the
`debug_swap` parameter of `kvm-amd.ko`.

### 2. KVM\_SEV\_LAUNCH\_START

The KVM\_SEV\_LAUNCH\_START command is used for creating the memory encryption
context. To create the encryption context, user must provide a guest policy,
the owner’s public Diffie-Hellman (PDH) key and session information.

Parameters: `struct  kvm_sev_launch_start` (in/out)

Returns: 0 on success, -negative on error

```
struct kvm_sev_launch_start {
        __u32 handle;           /* if zero then firmware creates a new handle */
        __u32 policy;           /* guest's policy */

        __u64 dh_uaddr;         /* userspace address pointing to the guest owner's PDH key */
        __u32 dh_len;

        __u64 session_addr;     /* userspace address which points to the guest session information */
        __u32 session_len;
};
```

On success, the ‘handle’ field contains a new handle and on error, a negative value.

KVM\_SEV\_LAUNCH\_START requires the `sev_fd` field to be valid.

For more details, see SEV spec Section 6.2.

### 3. KVM\_SEV\_LAUNCH\_UPDATE\_DATA

The KVM\_SEV\_LAUNCH\_UPDATE\_DATA is used for encrypting a memory region. It also
calculates a measurement of the memory contents. The measurement is a signature
of the memory contents that can be sent to the guest owner as an attestation
that the memory was encrypted correctly by the firmware.

Parameters (in): `struct  kvm_sev_launch_update_data`

Returns: 0 on success, -negative on error

```
struct kvm_sev_launch_update {
        __u64 uaddr;    /* userspace address to be encrypted (must be 16-byte aligned) */
        __u32 len;      /* length of the data to be encrypted (must be 16-byte aligned) */
};
```

For more details, see SEV spec Section 6.3.

### 4. KVM\_SEV\_LAUNCH\_MEASURE

The KVM\_SEV\_LAUNCH\_MEASURE command is used to retrieve the measurement of the
data encrypted by the KVM\_SEV\_LAUNCH\_UPDATE\_DATA command. The guest owner may
wait to provide the guest with confidential information until it can verify the
measurement. Since the guest owner knows the initial contents of the guest at
boot, the measurement can be verified by comparing it to what the guest owner
expects.

If len is zero on entry, the measurement blob length is written to len and
uaddr is unused.

Parameters (in): `struct  kvm_sev_launch_measure`

Returns: 0 on success, -negative on error

```
struct kvm_sev_launch_measure {
        __u64 uaddr;    /* where to copy the measurement */
        __u32 len;      /* length of measurement blob */
};
```

For more details on the measurement verification flow, see SEV spec Section 6.4.

### 5. KVM\_SEV\_LAUNCH\_FINISH

After completion of the launch flow, the KVM\_SEV\_LAUNCH\_FINISH command can be
issued to make the guest ready for the execution.

Returns: 0 on success, -negative on error

### 6. KVM\_SEV\_GUEST\_STATUS

The KVM\_SEV\_GUEST\_STATUS command is used to retrieve status information about a
SEV-enabled guest.

Parameters (out): `struct kvm_sev_guest_status`

Returns: 0 on success, -negative on error

```
struct kvm_sev_guest_status {
        __u32 handle;   /* guest handle */
        __u32 policy;   /* guest policy */
        __u8 state;     /* guest state (see enum below) */
};
```

SEV guest state:

```
enum {
SEV_STATE_INVALID = 0;
SEV_STATE_LAUNCHING,    /* guest is currently being launched */
SEV_STATE_SECRET,       /* guest is being launched and ready to accept the ciphertext data */
SEV_STATE_RUNNING,      /* guest is fully launched and running */
SEV_STATE_RECEIVING,    /* guest is being migrated in from another SEV machine */
SEV_STATE_SENDING       /* guest is getting migrated out to another SEV machine */
};
```

### 7. KVM\_SEV\_DBG\_DECRYPT

The KVM\_SEV\_DEBUG\_DECRYPT command can be used by the hypervisor to request the
firmware to decrypt the data at the given memory region.

Parameters (in): `struct kvm_sev_dbg`

Returns: 0 on success, -negative on error

```
struct kvm_sev_dbg {
        __u64 src_uaddr;        /* userspace address of data to decrypt */
        __u64 dst_uaddr;        /* userspace address of destination */
        __u32 len;              /* length of memory region to decrypt */
};
```

The command returns an error if the guest policy does not allow debugging.

### 8. KVM\_SEV\_DBG\_ENCRYPT

The KVM\_SEV\_DEBUG\_ENCRYPT command can be used by the hypervisor to request the
firmware to encrypt the data at the given memory region.

Parameters (in): `struct kvm_sev_dbg`

Returns: 0 on success, -negative on error

```
struct kvm_sev_dbg {
        __u64 src_uaddr;        /* userspace address of data to encrypt */
        __u64 dst_uaddr;        /* userspace address of destination */
        __u32 len;              /* length of memory region to encrypt */
};
```

The command returns an error if the guest policy does not allow debugging.

### 9. KVM\_SEV\_LAUNCH\_SECRET

The KVM\_SEV\_LAUNCH\_SECRET command can be used by the hypervisor to inject secret
data after the measurement has been validated by the guest owner.

Parameters (in): `struct kvm_sev_launch_secret`

Returns: 0 on success, -negative on error

```
struct kvm_sev_launch_secret {
        __u64 hdr_uaddr;        /* userspace address containing the packet header */
        __u32 hdr_len;

        __u64 guest_uaddr;      /* the guest memory region where the secret should be injected */
        __u32 guest_len;

        __u64 trans_uaddr;      /* the hypervisor memory region which contains the secret */
        __u32 trans_len;
};
```

### 10. KVM\_SEV\_GET\_ATTESTATION\_REPORT

The KVM\_SEV\_GET\_ATTESTATION\_REPORT command can be used by the hypervisor to query the attestation
report containing the SHA-256 digest of the guest memory and VMSA passed through the KVM\_SEV\_LAUNCH
commands and signed with the PEK. The digest returned by the command should match the digest
used by the guest owner with the KVM\_SEV\_LAUNCH\_MEASURE.

If len is zero on entry, the measurement blob length is written to len and
uaddr is unused.

Parameters (in): `struct kvm_sev_attestation`

Returns: 0 on success, -negative on error

```
struct kvm_sev_attestation_report {
        __u8 mnonce[16];        /* A random mnonce that will be placed in the report */

        __u64 uaddr;            /* userspace address where the report should be copied */
        __u32 len;
};
```

### 11. KVM\_SEV\_SEND\_START

The KVM\_SEV\_SEND\_START command can be used by the hypervisor to create an
outgoing guest encryption context.

If session\_len is zero on entry, the length of the guest session information is
written to session\_len and all other fields are not used.

Parameters (in): `struct kvm_sev_send_start`

Returns: 0 on success, -negative on error

```
struct kvm_sev_send_start {
        __u32 policy;                 /* guest policy */

        __u64 pdh_cert_uaddr;         /* platform Diffie-Hellman certificate */
        __u32 pdh_cert_len;

        __u64 plat_certs_uaddr;        /* platform certificate chain */
        __u32 plat_certs_len;

        __u64 amd_certs_uaddr;        /* AMD certificate */
        __u32 amd_certs_len;

        __u64 session_uaddr;          /* Guest session information */
        __u32 session_len;
};
```

### 12. KVM\_SEV\_SEND\_UPDATE\_DATA

The KVM\_SEV\_SEND\_UPDATE\_DATA command can be used by the hypervisor to encrypt the
outgoing guest memory region with the encryption context creating using
KVM\_SEV\_SEND\_START.

If hdr\_len or trans\_len are zero on entry, the length of the packet header and
transport region are written to hdr\_len and trans\_len respectively, and all
other fields are not used.

Parameters (in): `struct kvm_sev_send_update_data`

Returns: 0 on success, -negative on error

```
struct kvm_sev_launch_send_update_data {
        __u64 hdr_uaddr;        /* userspace address containing the packet header */
        __u32 hdr_len;

        __u64 guest_uaddr;      /* the source memory region to be encrypted */
        __u32 guest_len;

        __u64 trans_uaddr;      /* the destination memory region  */
        __u32 trans_len;
};
```

### 13. KVM\_SEV\_SEND\_FINISH

After completion of the migration flow, the KVM\_SEV\_SEND\_FINISH command can be
issued by the hypervisor to delete the encryption context.

Returns: 0 on success, -negative on error

### 14. KVM\_SEV\_SEND\_CANCEL

After completion of SEND\_START, but before SEND\_FINISH, the source VMM can issue the
SEND\_CANCEL command to stop a migration. This is necessary so that a cancelled
migration can restart with a new target later.

Returns: 0 on success, -negative on error

### 15. KVM\_SEV\_RECEIVE\_START

The KVM\_SEV\_RECEIVE\_START command is used for creating the memory encryption
context for an incoming SEV guest. To create the encryption context, the user must
provide a guest policy, the platform public Diffie-Hellman (PDH) key and session
information.

Parameters: `struct  kvm_sev_receive_start` (in/out)

Returns: 0 on success, -negative on error

```
struct kvm_sev_receive_start {
        __u32 handle;           /* if zero then firmware creates a new handle */
        __u32 policy;           /* guest's policy */

        __u64 pdh_uaddr;        /* userspace address pointing to the PDH key */
        __u32 pdh_len;

        __u64 session_uaddr;    /* userspace address which points to the guest session information */
        __u32 session_len;
};
```

On success, the ‘handle’ field contains a new handle and on error, a negative value.

For more details, see SEV spec Section 6.12.

### 16. KVM\_SEV\_RECEIVE\_UPDATE\_DATA

The KVM\_SEV\_RECEIVE\_UPDATE\_DATA command can be used by the hypervisor to copy
the incoming buffers into the guest memory region with encryption context
created during the KVM\_SEV\_RECEIVE\_START.

Parameters (in): `struct kvm_sev_receive_update_data`

Returns: 0 on success, -negative on error

```
struct kvm_sev_launch_receive_update_data {
        __u64 hdr_uaddr;        /* userspace address containing the packet header */
        __u32 hdr_len;

        __u64 guest_uaddr;      /* the destination guest memory region */
        __u32 guest_len;

        __u64 trans_uaddr;      /* the incoming buffer memory region  */
        __u32 trans_len;
};
```

### 17. KVM\_SEV\_RECEIVE\_FINISH

After completion of the migration flow, the KVM\_SEV\_RECEIVE\_FINISH command can be
issued by the hypervisor to make the guest ready for execution.

Returns: 0 on success, -negative on error

### 18. KVM\_SEV\_SNP\_LAUNCH\_START

The KVM\_SNP\_LAUNCH\_START command is used for creating the memory encryption
context for the SEV-SNP guest. It must be called prior to issuing
KVM\_SEV\_SNP\_LAUNCH\_UPDATE or KVM\_SEV\_SNP\_LAUNCH\_FINISH;

Parameters (in): `struct  kvm_sev_snp_launch_start`

Returns: 0 on success, -negative on error

```
struct kvm_sev_snp_launch_start {
        __u64 policy;           /* Guest policy to use. */
        __u8 gosvw[16];         /* Guest OS visible workarounds. */
        __u16 flags;            /* Must be zero. */
        __u8 pad0[6];
        __u64 pad1[4];
};
```

See SNP\_LAUNCH\_START in the SEV-SNP specification [[snp-fw-abi]](#snp-fw-abi) for further
details on the input parameters in `struct kvm_sev_snp_launch_start`.

### 19. KVM\_SEV\_SNP\_LAUNCH\_UPDATE

The KVM\_SEV\_SNP\_LAUNCH\_UPDATE command is used for loading userspace-provided
data into a guest GPA range, measuring the contents into the SNP guest context
created by KVM\_SEV\_SNP\_LAUNCH\_START, and then encrypting/validating that GPA
range so that it will be immediately readable using the encryption key
associated with the guest context once it is booted, after which point it can
attest the measurement associated with its context before unlocking any
secrets.

It is required that the GPA ranges initialized by this command have had the
KVM\_MEMORY\_ATTRIBUTE\_PRIVATE attribute set in advance. See the documentation
for KVM\_SET\_MEMORY\_ATTRIBUTES for more details on this aspect.

Upon success, this command is not guaranteed to have processed the entire
range requested. Instead, the `gfn_start`, `uaddr`, and `len` fields of
`struct kvm_sev_snp_launch_update` will be updated to correspond to the
remaining range that has yet to be processed. The caller should continue
calling this command until those fields indicate the entire range has been
processed, e.g. `len` is 0, `gfn_start` is equal to the last GFN in the
range plus 1, and `uaddr` is the last byte of the userspace-provided source
buffer address plus 1. In the case where `type` is KVM\_SEV\_SNP\_PAGE\_TYPE\_ZERO,
`uaddr` will be ignored completely.

Parameters (in): `struct  kvm_sev_snp_launch_update`

Returns: 0 on success, < 0 on error, -EAGAIN if caller should retry

```
struct kvm_sev_snp_launch_update {
        __u64 gfn_start;        /* Guest page number to load/encrypt data into. */
        __u64 uaddr;            /* 4k-aligned address of data to be loaded/encrypted. */
        __u64 len;              /* 4k-aligned length in bytes to copy into guest memory.*/
        __u8 type;              /* The type of the guest pages being initialized. */
        __u8 pad0;
        __u16 flags;            /* Must be zero. */
        __u32 pad1;
        __u64 pad2[4];

};
```

where the allowed values for page\_type are #define’d as:

```
KVM_SEV_SNP_PAGE_TYPE_NORMAL
KVM_SEV_SNP_PAGE_TYPE_ZERO
KVM_SEV_SNP_PAGE_TYPE_UNMEASURED
KVM_SEV_SNP_PAGE_TYPE_SECRETS
KVM_SEV_SNP_PAGE_TYPE_CPUID
```

See the SEV-SNP spec [[snp-fw-abi]](#snp-fw-abi) for further details on how each page type is
used/measured.

### 20. KVM\_SEV\_SNP\_LAUNCH\_FINISH

After completion of the SNP guest launch flow, the KVM\_SEV\_SNP\_LAUNCH\_FINISH
command can be issued to make the guest ready for execution.

Parameters (in): `struct kvm_sev_snp_launch_finish`

Returns: 0 on success, -negative on error

```
struct kvm_sev_snp_launch_finish {
        __u64 id_block_uaddr;
        __u64 id_auth_uaddr;
        __u8 id_block_en;
        __u8 auth_key_en;
        __u8 vcek_disabled;
        __u8 host_data[32];
        __u8 pad0[3];
        __u16 flags;                    /* Must be zero */
        __u64 pad1[4];
};
```

See SNP\_LAUNCH\_FINISH in the SEV-SNP specification [[snp-fw-abi]](#snp-fw-abi) for further
details on the input parameters in `struct kvm_sev_snp_launch_finish`.

### 21. KVM\_SEV\_SNP\_ENABLE\_REQ\_CERTS

The KVM\_SEV\_SNP\_ENABLE\_REQ\_CERTS command will configure KVM to exit to
userspace with a `KVM_EXIT_SNP_REQ_CERTS` exit type as part of handling
a guest attestation report, which will to allow userspace to provide a
certificate corresponding to the endorsement key used by firmware to sign
that attestation report.

Returns: 0 on success, -negative on error

NOTE: The endorsement key used by firmware may change as a result of
management activities like updating SEV-SNP firmware or loading new
endorsement keys, so some care should be taken to keep the returned
certificate data in sync with the actual endorsement key in use by
firmware at the time the attestation request is sent to SNP firmware. The
recommended scheme to do this is to use file locking (e.g. via fcntl()’s
F\_OFD\_SETLK) in the following manner:

> * Prior to obtaining/providing certificate data as part of servicing an
>   exit type of `KVM_EXIT_SNP_REQ_CERTS`, the VMM should obtain a
>   shared/read or exclusive/write lock on the certificate blob file before
>   reading it and returning it to KVM, and continue to hold the lock until
>   the attestation request is actually sent to firmware. To facilitate
>   this, the VMM can set the `immediate_exit` flag of kvm\_run just after
>   supplying the certificate data, and just before resuming the vCPU.
>   This will ensure the vCPU will exit again to userspace with `-EINTR`
>   after it finishes fetching the attestation request from firmware, at
>   which point the VMM can safely drop the file lock.
> * Tools/libraries that perform updates to SNP firmware TCB values or
>   endorsement keys (e.g. via /dev/sev interfaces such as `SNP_COMMIT`,
>   `SNP_SET_CONFIG`, or `SNP_VLEK_LOAD`, see
>   [The Definitive SEV Guest API Documentation](../../coco/sev-guest.html) for more details) in such a way
>   that the certificate blob needs to be updated, should similarly take an
>   exclusive lock on the certificate blob for the duration of any updates
>   to endorsement keys or the certificate blob contents to ensure that
>   VMMs using the above scheme will not return certificate blob data that
>   is out of sync with the endorsement key used by firmware at the time
>   the attestation request is actually issued.

This scheme is recommended so that tools can use a fairly generic/natural
approach to synchronizing firmware/certificate updates via file-locking,
which should make it easier to maintain interoperability across
tools/VMMs/vendors.

## Device attribute API

Attributes of the SEV implementation can be retrieved through the
`KVM_HAS_DEVICE_ATTR` and `KVM_GET_DEVICE_ATTR` ioctls on the `/dev/kvm`
device node, using group `KVM_X86_GRP_SEV`.

The following attributes are currently implemented:

* `KVM_X86_SEV_VMSA_FEATURES`: return the set of all bits that
  are accepted in the `vmsa_features` of `KVM_SEV_INIT2`.
* `KVM_X86_SEV_SNP_REQ_CERTS`: return a value of 1 if the kernel supports the
  `KVM_EXIT_SNP_REQ_CERTS` exit, which allows for fetching endorsement key
  certificates from userspace for each SNP attestation request the guest issues.

## Firmware Management

The SEV guest key management is handled by a separate processor called the AMD
Secure Processor (AMD-SP). Firmware running inside the AMD-SP provides a secure
key management interface to perform common hypervisor activities such as
encrypting bootstrap code, snapshot, migrating and debugging the guest. For more
information, see the SEV Key Management spec [[api-spec]](#api-spec)

The AMD-SP firmware can be initialized either by using its own non-volatile
storage or the OS can manage the NV storage for the firmware using
parameter `init_ex_path` of the `ccp` module. If the file specified
by `init_ex_path` does not exist or is invalid, the OS will create or
override the file with PSP non-volatile storage.

## References

See [[white-paper]](#white-paper), [[api-spec]](#api-spec), [[amd-apm]](#amd-apm), [[kvm-forum]](#kvm-forum), and [[snp-fw-abi]](#snp-fw-abi)
for more info.

[[white-paper](#id5)]

<https://docs.amd.com/v/u/en-US/memory-encryption-white-paper>

[api-spec]
([1](#id4),[2](#id6))

<https://docs.amd.com/v/u/en-US/55766_PUB_3.24_SEV_API>

[[amd-apm](#id7)]

<https://docs.amd.com/v/u/en-US/24593_3.44_APM_Vol2> (section 15.34)

[[kvm-forum](#id8)]

<https://www.linux-kvm.org/images/7/74/02x08A-Thomas_Lendacky-AMDs_Virtualizatoin_Memory_Encryption_Technology.pdf>

[snp-fw-abi]
([1](#id1),[2](#id2),[3](#id3),[4](#id9))

<https://www.amd.com/content/dam/amd/en/documents/developer/56860.pdf>
