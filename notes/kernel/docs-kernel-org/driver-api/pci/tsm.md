# PCI Trusted Execution Environment Security Manager (TSM)

> 출처(원문): https://docs.kernel.org/driver-api/pci/tsm.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# PCI Trusted Execution Environment Security Manager (TSM)

## Subsystem Interfaces

struct pci\_ide\_partner
:   Per port pair Selective IDE Stream settings

**Definition**:

```
struct pci_ide_partner {
    u16 rid_start;
    u16 rid_end;
    u8 stream_index;
    struct pci_bus_region mem_assoc;
    struct pci_bus_region pref_assoc;
    unsigned int default_stream:1;
    unsigned int setup:1;
    unsigned int enable:1;
};
```

**Members**

`rid_start`
:   Partner Port Requester ID range start

`rid_end`
:   Partner Port Requester ID range end (inclusive)

`stream_index`
:   Selective IDE Stream Register Block selection

`mem_assoc`
:   PCI bus memory address association for targeting peer partner

`pref_assoc`
:   PCI bus prefetchable memory address association for
    targeting peer partner

`default_stream`
:   Endpoint uses this stream for all upstream TLPs regardless of
    address and RID association registers

`setup`
:   flag to track whether to run [`pci_ide_stream_teardown()`](#c.pci_ide_stream_teardown "pci_ide_stream_teardown") for this
    partner slot

`enable`
:   flag whether to run [`pci_ide_stream_disable()`](#c.pci_ide_stream_disable "pci_ide_stream_disable") for this partner slot

**Description**

By default, [`pci_ide_stream_alloc()`](#c.pci_ide_stream_alloc "pci_ide_stream_alloc") initializes **mem\_assoc** and **pref\_assoc**
with the immediate ancestor downstream port memory ranges (i.e. Type 1
Configuration Space Header values). Caller may zero size ({0, -1}) the range
to drop it from consideration at [`pci_ide_stream_setup()`](#c.pci_ide_stream_setup "pci_ide_stream_setup") time.

struct pci\_ide\_regs
:   Hardware register association settings for Selective IDE Streams

**Definition**:

```
struct pci_ide_regs {
    u32 rid1;
    u32 rid2;
    struct {
        u32 assoc1;
        u32 assoc2;
        u32 assoc3;
    } addr[2];
    int nr_addr;
};
```

**Members**

`rid1`
:   IDE RID Association Register 1

`rid2`
:   IDE RID Association Register 2

`addr`
:   Up to two address association blocks (IDE Address Association Register
    1 through 3) for MMIO and prefetchable MMIO

`nr_addr`
:   Number of address association blocks initialized

**Description**

See `pci_ide_stream_to_regs()`

struct pci\_ide
:   PCIe Selective IDE Stream descriptor

**Definition**:

```
struct pci_ide {
    struct pci_dev *pdev;
    struct pci_ide_partner partner[PCI_IDE_PARTNER_MAX];
    u8 host_bridge_stream;
    int stream_id;
    const char *name;
};
```

**Members**

`pdev`
:   PCIe Endpoint in the pci\_ide\_partner pair

`partner`
:   per-partner settings

`host_bridge_stream`
:   allocated from host bridge **ide\_stream\_ida** pool

`stream_id`
:   unique Stream ID (within Partner Port pairing)

`name`
:   name of the established Selective IDE Stream in sysfs

**Description**

Negative **stream\_id** values indicate “uninitialized” on the
expectation that with TSM established IDE the TSM owns the stream\_id
allocation.

struct [pci\_ide](#c.pci_ide "pci_ide") \*pci\_ide\_stream\_alloc(struct pci\_dev \*pdev)
:   Reserve stream indices and probe for settings

**Parameters**

`struct pci_dev *pdev`
:   IDE capable PCIe Endpoint Physical Function

**Description**

Retrieve the Requester ID range of **pdev** for programming its Root
Port IDE RID Association registers, and conversely retrieve the
Requester ID of the Root Port for programming **pdev**’s IDE RID
Association registers.

Allocate a Selective IDE Stream Register Block instance per port.

Allocate a platform stream resource from the associated host bridge.
Retrieve stream association parameters for Requester ID range and
address range restrictions for the stream.

void pci\_ide\_stream\_free(struct [pci\_ide](#c.pci_ide "pci_ide") \*ide)
:   unwind [`pci_ide_stream_alloc()`](#c.pci_ide_stream_alloc "pci_ide_stream_alloc")

**Parameters**

`struct pci_ide *ide`
:   idle IDE settings descriptor

**Description**

Free all of the stream index (register block) allocations acquired by
[`pci_ide_stream_alloc()`](#c.pci_ide_stream_alloc "pci_ide_stream_alloc"). The stream represented by **ide** is assumed to
be unregistered and not instantiated in any device.

void pci\_ide\_stream\_release(struct [pci\_ide](#c.pci_ide "pci_ide") \*ide)
:   unwind and release an **ide** context

**Parameters**

`struct pci_ide *ide`
:   partially or fully registered IDE settings descriptor

**Description**

In support of automatic cleanup of IDE setup routines perform IDE
teardown in expected reverse order of setup and with respect to which
aspects of IDE setup have successfully completed.

Be careful that setup order mirrors this shutdown order. Otherwise,
open code releasing the IDE context.

int pci\_ide\_stream\_register(struct [pci\_ide](#c.pci_ide "pci_ide") \*ide)
:   Prepare to activate an IDE Stream

**Parameters**

`struct pci_ide *ide`
:   IDE settings descriptor

**Description**

After a Stream ID has been acquired for **ide**, record the presence of
the stream in sysfs. The expectation is that **ide** is immutable while
registered.

void pci\_ide\_stream\_unregister(struct [pci\_ide](#c.pci_ide "pci_ide") \*ide)
:   unwind [`pci_ide_stream_register()`](#c.pci_ide_stream_register "pci_ide_stream_register")

**Parameters**

`struct pci_ide *ide`
:   idle IDE settings descriptor

**Description**

In preparation for freeing **ide**, remove sysfs enumeration for the
stream.

void pci\_ide\_stream\_setup(struct pci\_dev \*pdev, struct [pci\_ide](#c.pci_ide "pci_ide") \*ide)
:   program settings to Selective IDE Stream registers

**Parameters**

`struct pci_dev *pdev`
:   PCIe device object for either a Root Port or Endpoint Partner Port

`struct pci_ide *ide`
:   registered IDE settings descriptor

**Description**

When **pdev** is a PCI\_EXP\_TYPE\_ENDPOINT then the PCI\_IDE\_EP partner
settings are written to **pdev**’s Selective IDE Stream register block,
and when **pdev** is a PCI\_EXP\_TYPE\_ROOT\_PORT, the PCI\_IDE\_RP settings
are selected.

void pci\_ide\_stream\_teardown(struct pci\_dev \*pdev, struct [pci\_ide](#c.pci_ide "pci_ide") \*ide)
:   disable the stream and clear all settings

**Parameters**

`struct pci_dev *pdev`
:   PCIe device object for either a Root Port or Endpoint Partner Port

`struct pci_ide *ide`
:   registered IDE settings descriptor

**Description**

For stream destruction, zero all registers that may have been written
by [`pci_ide_stream_setup()`](#c.pci_ide_stream_setup "pci_ide_stream_setup"). Consider [`pci_ide_stream_disable()`](#c.pci_ide_stream_disable "pci_ide_stream_disable") to leave
settings in place while temporarily disabling the stream.

int pci\_ide\_stream\_enable(struct pci\_dev \*pdev, struct [pci\_ide](#c.pci_ide "pci_ide") \*ide)
:   enable a Selective IDE Stream

**Parameters**

`struct pci_dev *pdev`
:   PCIe device object for either a Root Port or Endpoint Partner Port

`struct pci_ide *ide`
:   registered and setup IDE settings descriptor

**Description**

Activate the stream by writing to the Selective IDE Stream Control
Register.

Note that the state may go “insecure” at any point after returning 0, but
those events are equivalent to a “link down” event and handled via
asynchronous error reporting.

Caller is responsible to clear the enable bit in the -ENXIO case.

**Return**

0 if the stream successfully entered the “secure” state, and -EINVAL
if **ide** is invalid, and -ENXIO if the stream fails to enter the secure state.

void pci\_ide\_stream\_disable(struct pci\_dev \*pdev, struct [pci\_ide](#c.pci_ide "pci_ide") \*ide)
:   disable a Selective IDE Stream

**Parameters**

`struct pci_dev *pdev`
:   PCIe device object for either a Root Port or Endpoint Partner Port

`struct pci_ide *ide`
:   registered and setup IDE settings descriptor

**Description**

Clear the Selective IDE Stream Control Register, but leave all other
registers untouched.

void pci\_ide\_set\_nr\_streams(struct pci\_host\_bridge \*hb, u16 nr)
:   sets size of the pool of IDE Stream resources

**Parameters**

`struct pci_host_bridge *hb`
:   host bridge boundary for the stream pool

`u16 nr`
:   number of streams

**Description**

Platform PCI init and/or expert test module use only. Limit IDE
Stream establishment by setting the number of stream resources
available at the host bridge. Platform init code must set this before
the first [`pci_ide_stream_alloc()`](#c.pci_ide_stream_alloc "pci_ide_stream_alloc") call if the platform has less than the
default of 256 streams per host-bridge.

The “PCI\_IDE” symbol namespace is required because this is typically
a detail that is settled in early PCI init. I.e. this export is not
for endpoint drivers.

struct pci\_tdi
:   Core TEE I/O Device Interface (TDI) context

**Definition**:

```
struct pci_tdi {
    struct pci_dev *pdev;
    struct kvm *kvm;
    u32 tdi_id;
};
```

**Members**

`pdev`
:   host side representation of guest-side TDI

`kvm`
:   TEE VM context of bound TDI

`tdi_id`
:   Identifier (virtual BDF) for the TDI as referenced by the TSM and DSM

struct pci\_tsm
:   Core TSM context for a given PCIe endpoint

**Definition**:

```
struct pci_tsm {
    struct pci_dev *pdev;
    struct pci_dev *dsm_dev;
    struct tsm_dev *tsm_dev;
    struct pci_tdi *tdi;
};
```

**Members**

`pdev`
:   Back ref to device function, distinguishes type of pci\_tsm context

`dsm_dev`
:   PCI Device Security Manager for link operations on **pdev**

`tsm_dev`
:   PCI TEE Security Manager device for Link Confidentiality or Device
    Function Security operations

`tdi`
:   TDI context established by the **bind** link operation

**Description**

This structure is wrapped by low level TSM driver data and returned by
`probe()`/`lock()`, it is freed by the corresponding `remove()`/`unlock()`.

For link operations it serves to cache the association between a Device
Security Manager (DSM) and the functions that manager can assign to a TVM.
That can be “self”, for assigning function0 of a TEE I/O device, a
sub-function (SR-IOV virtual function, or non-function0
multifunction-device), or a downstream endpoint (PCIe upstream switch-port as
DSM).

struct pci\_tsm\_pf0
:   Physical Function 0 TDISP link context

**Definition**:

```
struct pci_tsm_pf0 {
    struct pci_tsm base_tsm;
    struct mutex lock;
    struct pci_doe_mb *doe_mb;
};
```

**Members**

`base_tsm`
:   generic core “tsm” context

`lock`
:   mutual exclustion for pci\_tsm\_ops invocation

`doe_mb`
:   PCIe Data Object Exchange mailbox

enum pci\_tsm\_req\_scope
:   Scope of guest requests to be validated by TSM

**Constants**

`PCI_TSM_REQ_INFO`
:   Read-only, without side effects, request for
    typical TDISP collateral information like Device Interface Reports.
    No device secrets are permitted, and no device state is changed.

`PCI_TSM_REQ_STATE_CHANGE`
:   Request to change the TDISP state from
    UNLOCKED->LOCKED, LOCKED->RUN, or other architecture specific state
    changes to support those transitions for a TDI. No other (unrelated
    to TDISP) device / host state, configuration, or data change is
    permitted.

`PCI_TSM_REQ_DEBUG_READ`
:   Read-only request for debug information

    A method to facilitate TVM information retrieval outside of typical
    TDISP operational requirements. No device secrets are permitted.

`PCI_TSM_REQ_DEBUG_WRITE`
:   Device state changes for debug purposes

    The request may affect the operational state of the device outside of
    the TDISP operational model. If allowed, requires CAP\_SYS\_RAW\_IO, and
    will taint the kernel.

**Description**

Guest requests are a transport for a TVM to communicate with a TSM + DSM for
a given TDI. A TSM driver is responsible for maintaining the kernel security
model and limit commands that may affect the host, or are otherwise outside
the typical TDISP operational model.

int pci\_tsm\_bind(struct pci\_dev \*pdev, struct [kvm](#c.pci_tsm_bind "kvm") \*kvm, u32 tdi\_id)
:   Bind **pdev** as a TDI for **kvm**

**Parameters**

`struct pci_dev *pdev`
:   PCI device function to bind

`struct kvm *kvm`
:   Private memory attach context

`u32 tdi_id`
:   Identifier (virtual BDF) for the TDI as referenced by the TSM and DSM

**Description**

Returns 0 on success, or a negative error code on failure.

**Context**

Caller is responsible for constraining the bind lifetime to the
registered state of the device. For example, [`pci_tsm_bind()`](#c.pci_tsm_bind "pci_tsm_bind") /
`pci_tsm_unbind()` limited to the VFIO driver bound state of the device.

ssize\_t pci\_tsm\_guest\_req(struct pci\_dev \*pdev, enum [pci\_tsm\_req\_scope](#c.pci_tsm_req_scope "pci_tsm_req_scope") scope, sockptr\_t req\_in, size\_t in\_len, sockptr\_t req\_out, size\_t out\_len, u64 \*tsm\_code)
:   helper to marshal guest requests to the TSM driver

**Parameters**

`struct pci_dev *pdev`
:   **pdev** representing a bound tdi

`enum pci_tsm_req_scope scope`
:   caller asserts this passthrough request is limited to TDISP operations

`sockptr_t req_in`
:   Input payload forwarded from the guest

`size_t in_len`
:   Length of **req\_in**

`sockptr_t req_out`
:   Output payload buffer response to the guest

`size_t out_len`
:   Length of **req\_out** on input, bytes filled in **req\_out** on output

`u64 *tsm_code`
:   Optional TSM arch specific result code for the guest TSM

**Description**

This is a common entry point for requests triggered by userspace KVM-exit
service handlers responding to TDI information or state change requests. The
scope parameter limits requests to TDISP state management, or limited debug.
This path is only suitable for commands and results that are the host kernel
has no use, the host is only facilitating guest to TSM communication.

Returns 0 on success and -error on failure and positive “residue” on success
but **req\_out** is filled with less then **out\_len**, or **req\_out** is NULL and a
residue number of bytes were not consumed from **req\_in**. On success or
failure **tsm\_code** may be populated with a TSM implementation specific result
code for the guest to consume.

**Context**

Caller is responsible for calling this within the [`pci_tsm_bind()`](#c.pci_tsm_bind "pci_tsm_bind")
state of the TDI.

void pci\_tsm\_tdi\_constructor(struct pci\_dev \*pdev, struct [pci\_tdi](#c.pci_tdi "pci_tdi") \*tdi, struct [kvm](#c.pci_tsm_tdi_constructor "kvm") \*kvm, u32 tdi\_id)
:   base ‘[`struct pci_tdi`](#c.pci_tdi "pci_tdi")’ initialization for link TSMs

**Parameters**

`struct pci_dev *pdev`
:   PCI device function representing the TDI

`struct pci_tdi *tdi`
:   context to initialize

`struct kvm *kvm`
:   Private memory attach context

`u32 tdi_id`
:   Identifier (virtual BDF) for the TDI as referenced by the TSM and DSM

int pci\_tsm\_link\_constructor(struct pci\_dev \*pdev, struct [pci\_tsm](#c.pci_tsm "pci_tsm") \*tsm, struct [tsm\_dev](#c.pci_tsm_link_constructor "tsm_dev") \*tsm\_dev)
:   base ‘[`struct pci_tsm`](#c.pci_tsm "pci_tsm")’ initialization for link TSMs

**Parameters**

`struct pci_dev *pdev`
:   The PCI device

`struct pci_tsm *tsm`
:   context to initialize

`struct tsm_dev *tsm_dev`
:   Platform TEE Security Manager, initiator of security operations

int pci\_tsm\_pf0\_constructor(struct pci\_dev \*pdev, struct [pci\_tsm\_pf0](#c.pci_tsm_pf0 "pci_tsm_pf0") \*tsm, struct [tsm\_dev](#c.pci_tsm_pf0_constructor "tsm_dev") \*tsm\_dev)
:   common ‘[`struct pci_tsm_pf0`](#c.pci_tsm_pf0 "pci_tsm_pf0")’ (DSM) initialization

**Parameters**

`struct pci_dev *pdev`
:   Physical Function 0 PCI device (as indicated by `is_pci_tsm_pf0()`)

`struct pci_tsm_pf0 *tsm`
:   context to initialize

`struct tsm_dev *tsm_dev`
:   Platform TEE Security Manager, initiator of security operations
