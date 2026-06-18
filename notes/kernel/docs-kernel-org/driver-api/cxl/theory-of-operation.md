# Compute Express Link Driver Theory of Operation

> 출처(원문): https://docs.kernel.org/driver-api/cxl/theory-of-operation.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Compute Express Link Driver Theory of Operation

A Compute Express Link Memory Device is a CXL component that implements the
CXL.mem protocol. It contains some amount of volatile memory, persistent memory,
or both. It is enumerated as a PCI device for configuration and passing
messages over an MMIO mailbox. Its contribution to the System Physical
Address space is handled via HDM (Host Managed Device Memory) decoders
that optionally define a device’s contribution to an interleaved address
range across multiple devices underneath a host-bridge or interleaved
across host-bridges.

## The CXL Bus

Similar to how a RAID driver takes disk objects and assembles them into a new
logical device, the CXL subsystem is tasked to take PCIe and ACPI objects and
assemble them into a CXL.mem decode topology. The need for runtime configuration
of the CXL.mem topology is also similar to RAID in that different environments
with the same hardware configuration may decide to assemble the topology in
contrasting ways. One may choose performance (RAID0) striping memory across
multiple Host Bridges and endpoints while another may opt for fault tolerance
and disable any striping in the CXL.mem topology.

Platform firmware enumerates a menu of interleave options at the “CXL root port”
(Linux term for the top of the CXL decode topology). From there, PCIe topology
dictates which endpoints can participate in which Host Bridge decode regimes.
Each PCIe Switch in the path between the root and an endpoint introduces a point
at which the interleave can be split. For example, platform firmware may say a
given range only decodes to one Host Bridge, but that Host Bridge may in turn
interleave cycles across multiple Root Ports. An intervening Switch between a
port and an endpoint may interleave cycles across multiple Downstream Switch
Ports, etc.

Here is a sample listing of a CXL topology defined by ‘cxl\_test’. The ‘cxl\_test’
module generates an emulated CXL topology of 2 Host Bridges each with 2 Root
Ports. Each of those Root Ports are connected to 2-way switches with endpoints
connected to those downstream ports for a total of 8 endpoints:

```
# cxl list -BEMPu -b cxl_test
{
  "bus":"root3",
  "provider":"cxl_test",
  "ports:root3":[
    {
      "port":"port5",
      "host":"cxl_host_bridge.1",
      "ports:port5":[
        {
          "port":"port8",
          "host":"cxl_switch_uport.1",
          "endpoints:port8":[
            {
              "endpoint":"endpoint9",
              "host":"mem2",
              "memdev":{
                "memdev":"mem2",
                "pmem_size":"256.00 MiB (268.44 MB)",
                "ram_size":"256.00 MiB (268.44 MB)",
                "serial":"0x1",
                "numa_node":1,
                "host":"cxl_mem.1"
              }
            },
            {
              "endpoint":"endpoint15",
              "host":"mem6",
              "memdev":{
                "memdev":"mem6",
                "pmem_size":"256.00 MiB (268.44 MB)",
                "ram_size":"256.00 MiB (268.44 MB)",
                "serial":"0x5",
                "numa_node":1,
                "host":"cxl_mem.5"
              }
            }
          ]
        },
        {
          "port":"port12",
          "host":"cxl_switch_uport.3",
          "endpoints:port12":[
            {
              "endpoint":"endpoint17",
              "host":"mem8",
              "memdev":{
                "memdev":"mem8",
                "pmem_size":"256.00 MiB (268.44 MB)",
                "ram_size":"256.00 MiB (268.44 MB)",
                "serial":"0x7",
                "numa_node":1,
                "host":"cxl_mem.7"
              }
            },
            {
              "endpoint":"endpoint13",
              "host":"mem4",
              "memdev":{
                "memdev":"mem4",
                "pmem_size":"256.00 MiB (268.44 MB)",
                "ram_size":"256.00 MiB (268.44 MB)",
                "serial":"0x3",
                "numa_node":1,
                "host":"cxl_mem.3"
              }
            }
          ]
        }
      ]
    },
    {
      "port":"port4",
      "host":"cxl_host_bridge.0",
      "ports:port4":[
        {
          "port":"port6",
          "host":"cxl_switch_uport.0",
          "endpoints:port6":[
            {
              "endpoint":"endpoint7",
              "host":"mem1",
              "memdev":{
                "memdev":"mem1",
                "pmem_size":"256.00 MiB (268.44 MB)",
                "ram_size":"256.00 MiB (268.44 MB)",
                "serial":"0",
                "numa_node":0,
                "host":"cxl_mem.0"
              }
            },
            {
              "endpoint":"endpoint14",
              "host":"mem5",
              "memdev":{
                "memdev":"mem5",
                "pmem_size":"256.00 MiB (268.44 MB)",
                "ram_size":"256.00 MiB (268.44 MB)",
                "serial":"0x4",
                "numa_node":0,
                "host":"cxl_mem.4"
              }
            }
          ]
        },
        {
          "port":"port10",
          "host":"cxl_switch_uport.2",
          "endpoints:port10":[
            {
              "endpoint":"endpoint16",
              "host":"mem7",
              "memdev":{
                "memdev":"mem7",
                "pmem_size":"256.00 MiB (268.44 MB)",
                "ram_size":"256.00 MiB (268.44 MB)",
                "serial":"0x6",
                "numa_node":0,
                "host":"cxl_mem.6"
              }
            },
            {
              "endpoint":"endpoint11",
              "host":"mem3",
              "memdev":{
                "memdev":"mem3",
                "pmem_size":"256.00 MiB (268.44 MB)",
                "ram_size":"256.00 MiB (268.44 MB)",
                "serial":"0x2",
                "numa_node":0,
                "host":"cxl_mem.2"
              }
            }
          ]
        }
      ]
    }
  ]
}
```

In that listing each “root”, “port”, and “endpoint” object correspond a kernel
‘[`struct cxl_port`](#c.cxl_port "cxl_port")’ object. A ‘cxl\_port’ is a device that can decode CXL.mem to
its descendants. So “root” claims non-PCIe enumerable platform decode ranges and
decodes them to “ports”, “ports” decode to “endpoints”, and “endpoints”
represent the decode from SPA (System Physical Address) to DPA (Device Physical
Address).

Continuing the RAID analogy, disks have both topology metadata and on-device
metadata that determine RAID set assembly. CXL Port topology and CXL Port link
status is metadata for CXL.mem set assembly. The CXL Port topology is enumerated
by the arrival of a CXL.mem device. I.e. unless and until the PCIe core attaches
the cxl\_pci driver to a CXL Memory Expander there is no role for CXL Port
objects. Conversely for hot-unplug / removal scenarios, there is no need for
the Linux PCI core to tear down switch-level CXL resources because the endpoint
->`remove()` event cleans up the port data that was established to support that
Memory Expander.

The port metadata and potential decode schemes that a given memory device may
participate can be determined via a command like:

```
# cxl list -BDMu -d root -m mem3
{
  "bus":"root3",
  "provider":"cxl_test",
  "decoders:root3":[
    {
      "decoder":"decoder3.1",
      "resource":"0x8030000000",
      "size":"512.00 MiB (536.87 MB)",
      "volatile_capable":true,
      "nr_targets":2
    },
    {
      "decoder":"decoder3.3",
      "resource":"0x8060000000",
      "size":"512.00 MiB (536.87 MB)",
      "pmem_capable":true,
      "nr_targets":2
    },
    {
      "decoder":"decoder3.0",
      "resource":"0x8020000000",
      "size":"256.00 MiB (268.44 MB)",
      "volatile_capable":true,
      "nr_targets":1
    },
    {
      "decoder":"decoder3.2",
      "resource":"0x8050000000",
      "size":"256.00 MiB (268.44 MB)",
      "pmem_capable":true,
      "nr_targets":1
    }
  ],
  "memdevs:root3":[
    {
      "memdev":"mem3",
      "pmem_size":"256.00 MiB (268.44 MB)",
      "ram_size":"256.00 MiB (268.44 MB)",
      "serial":"0x2",
      "numa_node":0,
      "host":"cxl_mem.2"
    }
  ]
}
```

...which queries the CXL topology to ask “given CXL Memory Expander with a kernel
device name of ‘mem3’ which platform level decode ranges may this device
participate”. A given expander can participate in multiple CXL.mem interleave
sets simultaneously depending on how many decoder resources it has. In this
example mem3 can participate in one or more of a PMEM interleave that spans two
Host Bridges, a PMEM interleave that targets a single Host Bridge, a Volatile
memory interleave that spans 2 Host Bridges, and a Volatile memory interleave
that only targets a single Host Bridge.

Conversely the memory devices that can participate in a given platform level
decode scheme can be determined via a command like the following:

```
# cxl list -MDu -d 3.2
[
  {
    "memdevs":[
      {
        "memdev":"mem1",
        "pmem_size":"256.00 MiB (268.44 MB)",
        "ram_size":"256.00 MiB (268.44 MB)",
        "serial":"0",
        "numa_node":0,
        "host":"cxl_mem.0"
      },
      {
        "memdev":"mem5",
        "pmem_size":"256.00 MiB (268.44 MB)",
        "ram_size":"256.00 MiB (268.44 MB)",
        "serial":"0x4",
        "numa_node":0,
        "host":"cxl_mem.4"
      },
      {
        "memdev":"mem7",
        "pmem_size":"256.00 MiB (268.44 MB)",
        "ram_size":"256.00 MiB (268.44 MB)",
        "serial":"0x6",
        "numa_node":0,
        "host":"cxl_mem.6"
      },
      {
        "memdev":"mem3",
        "pmem_size":"256.00 MiB (268.44 MB)",
        "ram_size":"256.00 MiB (268.44 MB)",
        "serial":"0x2",
        "numa_node":0,
        "host":"cxl_mem.2"
      }
    ]
  },
  {
    "root decoders":[
      {
        "decoder":"decoder3.2",
        "resource":"0x8050000000",
        "size":"256.00 MiB (268.44 MB)",
        "pmem_capable":true,
        "nr_targets":1
      }
    ]
  }
]
```

...where the naming scheme for decoders is “decoder<port\_id>.<instance\_id>”.

## Driver Infrastructure

This section covers the driver infrastructure for a CXL memory device.

### CXL Memory Device

This implements the PCI exclusive functionality for a CXL device as it is
defined by the Compute Express Link specification. CXL devices may surface
certain functionality even if it isn’t CXL enabled. While this driver is
focused around the PCI specific aspects of a CXL device, it binds to the
specific CXL memory device class code, and therefore the implementation of
cxl\_pci is focused around CXL memory devices.

The driver has several responsibilities, mainly:
:   * Create the memX device and register on the CXL bus.
    * Enumerate device’s register interface and map them.
    * Registers nvdimm bridge device with cxl\_core.
    * Registers a CXL mailbox with cxl\_core.

int \_\_cxl\_pci\_mbox\_send\_cmd(struct cxl\_mailbox \*cxl\_mbox, struct cxl\_mbox\_cmd \*mbox\_cmd)
:   Execute a mailbox command

**Parameters**

`struct cxl_mailbox *cxl_mbox`
:   CXL mailbox context

`struct cxl_mbox_cmd *mbox_cmd`
:   Command to send to the memory device.

**Context**

Any context. Expects mbox\_mutex to be held.

**Return**

-ETIMEDOUT if timeout occurred waiting for completion. 0 on success.
Caller should check the return code in **mbox\_cmd** to make sure it
succeeded.

**Description**

This is a generic form of the CXL mailbox send command thus only using the
registers defined by the mailbox capability ID - CXL 2.0 8.2.8.4. Memory
devices, and perhaps other types of CXL devices may have further information
available upon error conditions. Driver facilities wishing to send mailbox
commands should use the wrapper command.

The CXL spec allows for up to two mailboxes. The intention is for the primary
mailbox to be OS controlled and the secondary mailbox to be used by system
firmware. This allows the OS and firmware to communicate with the device and
not need to coordinate with each other. The driver only uses the primary
mailbox.

CXL memory endpoint devices and switches are CXL capable devices that are
participating in CXL.mem protocol. Their functionality builds on top of the
CXL.io protocol that allows enumerating and configuring components via
standard PCI mechanisms.

The cxl\_mem driver owns kicking off the enumeration of this CXL.mem
capability. With the detection of a CXL capable endpoint, the driver will
walk up to find the platform specific port it is connected to, and determine
if there are intervening switches in the path. If there are switches, a
secondary action is to enumerate those (implemented in cxl\_core). Finally the
cxl\_mem driver adds the device it is bound to as a CXL endpoint-port for use
in higher level operations.

struct cxl\_memdev
:   CXL bus object representing a Type-3 Memory Device

**Definition**:

```
struct cxl_memdev {
    struct device dev;
    struct cdev cdev;
    struct cxl_dev_state *cxlds;
    struct work_struct detach_work;
    struct cxl_nvdimm_bridge *cxl_nvb;
    struct cxl_nvdimm *cxl_nvd;
    struct cxl_port *endpoint;
    const struct cxl_memdev_attach *attach;
    int id;
    int depth;
    u8 scrub_cycle;
    int scrub_region_id;
    struct cxl_mem_err_rec *err_rec_array;
};
```

**Members**

`dev`
:   driver core device object

`cdev`
:   char dev core object for ioctl operations

`cxlds`
:   The device state backing this device

`detach_work`
:   active memdev lost a port in its ancestry

`cxl_nvb`
:   coordinate removal of **cxl\_nvd** if present

`cxl_nvd`
:   optional bridge to an nvdimm if the device supports pmem

`endpoint`
:   connection to the CXL port topology for this memory device

`attach`
:   creator of this memdev depends on CXL link attach to operate

`id`
:   id number of this memdev instance.

`depth`
:   endpoint port depth

`scrub_cycle`
:   current scrub cycle set for this device

`scrub_region_id`
:   id number of a backed region (if any) for which current scrub cycle set

`err_rec_array`
:   List of xarrarys to store the memdev error records to
    check attributes for a memory repair operation are from
    current boot.

struct cxl\_event\_state
:   Event log driver state

**Definition**:

```
struct cxl_event_state {
    struct cxl_get_event_payload *buf;
    struct mutex log_lock;
};
```

**Members**

`buf`
:   Buffer to receive event data

`log_lock`
:   Serialize event\_buf and log use

struct cxl\_poison\_state
:   Driver poison state info

**Definition**:

```
struct cxl_poison_state {
    u32 max_errors;
    unsigned long enabled_cmds[BITS_TO_LONGS( CXL_POISON_ENABLED_MAX)];
    struct cxl_mbox_poison_out *list_out;
    struct mutex mutex;
};
```

**Members**

`max_errors`
:   Maximum media error records held in device cache

`enabled_cmds`
:   All poison commands enabled in the CEL

`list_out`
:   The poison list payload returned by device

`mutex`
:   Protect reads of the poison list

**Description**

Reads of the poison list are synchronized to ensure that a reader
does not get an incomplete list because their request overlapped
(was interrupted or preceded by) another read request of the same
DPA range. CXL Spec 3.0 Section 8.2.9.8.4.1

struct cxl\_fw\_state
:   Firmware upload / activation state

**Definition**:

```
struct cxl_fw_state {
    unsigned long state[BITS_TO_LONGS( CXL_FW_STATE_BITS)];
    bool oneshot;
    int num_slots;
    int cur_slot;
    int next_slot;
};
```

**Members**

`state`
:   fw\_uploader state bitmask

`oneshot`
:   whether the fw upload fits in a single transfer

`num_slots`
:   Number of FW slots available

`cur_slot`
:   Slot number currently active

`next_slot`
:   Slot number for the new firmware

struct cxl\_security\_state
:   Device security state

**Definition**:

```
struct cxl_security_state {
    unsigned long state;
    unsigned long enabled_cmds[BITS_TO_LONGS( CXL_SEC_ENABLED_MAX)];
    int poll_tmo_secs;
    bool sanitize_active;
    struct delayed_work poll_dwork;
    struct kernfs_node *sanitize_node;
};
```

**Members**

`state`
:   state of last security operation

`enabled_cmds`
:   All security commands enabled in the CEL

`poll_tmo_secs`
:   polling timeout

`sanitize_active`
:   sanitize completion pending

`poll_dwork`
:   polling work item

`sanitize_node`
:   sanitation sysfs file to notify

struct cxl\_memdev\_state
:   Generic Type-3 Memory Device Class driver data

**Definition**:

```
struct cxl_memdev_state {
    struct cxl_dev_state cxlds;
    size_t lsa_size;
    char firmware_version[0x10];
    u64 total_bytes;
    u64 volatile_only_bytes;
    u64 persistent_only_bytes;
    u64 partition_align_bytes;
    u64 active_volatile_bytes;
    u64 active_persistent_bytes;
    struct cxl_event_state event;
    struct cxl_poison_state poison;
    struct cxl_security_state security;
    struct cxl_fw_state fw;
    struct notifier_block mce_notifier;
};
```

**Members**

`cxlds`
:   Core driver state common across Type-2 and Type-3 devices

`lsa_size`
:   Size of Label Storage Area
    (CXL 2.0 8.2.9.5.1.1 Identify Memory Device)

`firmware_version`
:   Firmware version for the memory device.

`total_bytes`
:   sum of all possible capacities

`volatile_only_bytes`
:   hard volatile capacity

`persistent_only_bytes`
:   hard persistent capacity

`partition_align_bytes`
:   alignment size for partition-able capacity

`active_volatile_bytes`
:   sum of hard + soft volatile

`active_persistent_bytes`
:   sum of hard + soft persistent

`event`
:   event log driver state

`poison`
:   poison driver state info

`security`
:   security driver state info

`fw`
:   firmware upload / activation state

`mce_notifier`
:   MCE notifier

**Description**

CXL 8.1.12.1 PCI Header - Class Code Register Memory Device defines
common memory device functionality like the presence of a mailbox and
the functionality related to that like Identify Memory Device and Get
Partition Info

See CXL 3.0 8.2.9.8.2 Capacity Configuration and Label Storage for
details on capacity parameters.

struct cxl\_mem\_command
:   Driver representation of a memory device command

**Definition**:

```
struct cxl_mem_command {
    struct cxl_command_info info;
    enum cxl_opcode opcode;
    u32 flags;
#define CXL_CMD_FLAG_FORCE_ENABLE BIT(0);
};
```

**Members**

`info`
:   Command information as it exists for the UAPI

`opcode`
:   The actual bits used for the mailbox protocol

`flags`
:   Set of flags effecting driver behavior.

**Description**

> * `CXL_CMD_FLAG_FORCE_ENABLE`: In cases of error, commands with this flag
>   will be enabled by the driver regardless of what hardware may have
>   advertised.

The cxl\_mem\_command is the driver’s internal representation of commands that
are supported by the driver. Some of these commands may not be supported by
the hardware. The driver will use **info** to validate the fields passed in by
the user then submit the **opcode** to the hardware.

See [`struct cxl_command_info`](#c.cxl_command_info "cxl_command_info").

struct cxl\_hdm
:   HDM Decoder registers and cached / decoded capabilities

**Definition**:

```
struct cxl_hdm {
    struct cxl_component_regs regs;
    int decoder_count;
    unsigned int target_count;
    unsigned int interleave_mask;
    unsigned long iw_cap_mask;
    struct cxl_port *port;
};
```

**Members**

`regs`
:   mapped registers, see [`devm_cxl_setup_hdm()`](#c.devm_cxl_setup_hdm "devm_cxl_setup_hdm")

`decoder_count`
:   number of decoders for this port

`target_count`
:   for switch decoders, max downstream port targets

`interleave_mask`
:   interleave granularity capability, see `check_interleave_cap()`

`iw_cap_mask`
:   bitmask of supported interleave ways, see `check_interleave_cap()`

`port`
:   mapped cxl\_port, see [`devm_cxl_setup_hdm()`](#c.devm_cxl_setup_hdm "devm_cxl_setup_hdm")

void set\_exclusive\_cxl\_commands(struct [cxl\_memdev\_state](#c.cxl_memdev_state "cxl_memdev_state") \*mds, unsigned long \*cmds)
:   atomically disable user cxl commands

**Parameters**

`struct cxl_memdev_state *mds`
:   The device state to operate on

`unsigned long *cmds`
:   bitmap of commands to mark exclusive

**Description**

Grab the cxl\_memdev\_rwsem in write mode to flush in-flight
invocations of the ioctl path and then disable future execution of
commands with the command ids set in **cmds**.

void clear\_exclusive\_cxl\_commands(struct [cxl\_memdev\_state](#c.cxl_memdev_state "cxl_memdev_state") \*mds, unsigned long \*cmds)
:   atomically enable user cxl commands

**Parameters**

`struct cxl_memdev_state *mds`
:   The device state to modify

`unsigned long *cmds`
:   bitmap of commands to mark available for userspace

int cxl\_mem\_get\_fw\_info(struct [cxl\_memdev\_state](#c.cxl_memdev_state "cxl_memdev_state") \*mds)
:   Get Firmware info

**Parameters**

`struct cxl_memdev_state *mds`
:   The device data for the operation

**Description**

Retrieve firmware info for the device specified.

See CXL-3.0 8.2.9.3.1 Get FW Info

**Return**

0 if no error: or the result of the mailbox command.

int cxl\_mem\_activate\_fw(struct [cxl\_memdev\_state](#c.cxl_memdev_state "cxl_memdev_state") \*mds, int slot)
:   Activate Firmware

**Parameters**

`struct cxl_memdev_state *mds`
:   The device data for the operation

`int slot`
:   slot number to activate

**Description**

Activate firmware in a given slot for the device specified.

See CXL-3.0 8.2.9.3.3 Activate FW

**Return**

0 if no error: or the result of the mailbox command.

int cxl\_mem\_abort\_fw\_xfer(struct [cxl\_memdev\_state](#c.cxl_memdev_state "cxl_memdev_state") \*mds)
:   Abort an in-progress FW transfer

**Parameters**

`struct cxl_memdev_state *mds`
:   The device data for the operation

**Description**

Abort an in-progress firmware transfer for the device specified.

See CXL-3.0 8.2.9.3.2 Transfer FW

**Return**

0 if no error: or the result of the mailbox command.

### CXL Port

The port driver enumerates dport via PCI and scans for HDM
(Host-managed-Device-Memory) decoder resources via the
**component\_reg\_phys** value passed in by the agent that registered the
port. All descendant ports of a CXL root port (described by platform
firmware) are managed in this drivers context. Each driver instance
is responsible for tearing down the driver context of immediate
descendant ports. The locking for this is validated by
CONFIG\_PROVE\_CXL\_LOCKING.

The primary service this driver provides is presenting APIs to other
drivers to utilize the decoders, and indicating to userspace (via bind
status) the connectivity of the CXL.mem protocol throughout the
PCIe topology.

### CXL Core

The CXL core objects like ports, decoders, and regions are shared
between the subsystem drivers cxl\_acpi, cxl\_pci, and core drivers
(port-driver, region-driver, nvdimm object-drivers... etc).

struct cxl\_decoder
:   Common CXL HDM Decoder Attributes

**Definition**:

```
struct cxl_decoder {
    struct device dev;
    int id;
    struct range hpa_range;
    int interleave_ways;
    int interleave_granularity;
    enum cxl_decoder_type target_type;
    struct cxl_region *region;
    unsigned long flags;
    u32 target_map[CXL_DECODER_MAX_INTERLEAVE];
    int (*commit)(struct cxl_decoder *cxld);
    void (*reset)(struct cxl_decoder *cxld);
};
```

**Members**

`dev`
:   this decoder’s device

`id`
:   kernel device name id

`hpa_range`
:   Host physical address range mapped by this decoder

`interleave_ways`
:   number of cxl\_dports in this decode

`interleave_granularity`
:   data stride per dport

`target_type`
:   accelerator vs expander (type2 vs type3) selector

`region`
:   currently assigned region for this decoder

`flags`
:   memory type capabilities and locking

`target_map`
:   cached copy of hardware port-id list, available at init
    before all **dport** objects have been instantiated. While
    dport id is 8bit, CFMWS interleave targets are 32bits.

`commit`
:   device/decoder-type specific callback to commit settings to hw

`reset`
:   device/decoder-type specific callback to reset hw settings

struct cxl\_endpoint\_decoder
:   Endpoint / SPA to DPA decoder

**Definition**:

```
struct cxl_endpoint_decoder {
    struct cxl_decoder cxld;
    struct resource *dpa_res;
    resource_size_t skip;
    enum cxl_decoder_state state;
    int part;
    int pos;
};
```

**Members**

`cxld`
:   base cxl\_decoder\_object

`dpa_res`
:   actively claimed DPA span of this decoder

`skip`
:   offset into **dpa\_res** where **cxld.hpa\_range** maps

`state`
:   autodiscovery state

`part`
:   partition index this decoder maps

`pos`
:   interleave position in **cxld.region**

struct cxl\_switch\_decoder
:   Switch specific CXL HDM Decoder

**Definition**:

```
struct cxl_switch_decoder {
    struct cxl_decoder cxld;
    int nr_targets;
    struct cxl_dport *target[];
};
```

**Members**

`cxld`
:   base cxl\_decoder object

`nr_targets`
:   number of elements in **target**

`target`
:   active ordered target list in current decoder configuration

**Description**

The ‘switch’ decoder type represents the decoder instances of cxl\_port’s that
route from the root of a CXL memory decode topology to the endpoints. They
come in two flavors, root-level decoders, statically defined by platform
firmware, and mid-level decoders, where interleave-granularity,
interleave-width, and the target list are mutable.

struct cxl\_rd\_ops
:   CXL root decoder callback operations

**Definition**:

```
struct cxl_rd_ops {
    u64 (*hpa_to_spa)(struct cxl_root_decoder *cxlrd, u64 hpa);
    u64 (*spa_to_hpa)(struct cxl_root_decoder *cxlrd, u64 spa);
};
```

**Members**

`hpa_to_spa`
:   Convert host physical address to system physical address

`spa_to_hpa`
:   Convert system physical address to host physical address

struct cxl\_root\_decoder
:   Static platform CXL address decoder

**Definition**:

```
struct cxl_root_decoder {
    struct resource *res;
    resource_size_t cache_size;
    atomic_t region_id;
    void *platform_data;
    struct mutex range_lock;
    int qos_class;
    struct cxl_rd_ops ops;
    struct cxl_switch_decoder cxlsd;
};
```

**Members**

`res`
:   host / parent resource for region allocations

`cache_size`
:   extended linear cache size if exists, otherwise zero.

`region_id`
:   region id for next region provisioning event

`platform_data`
:   platform specific configuration data

`range_lock`
:   sync region autodiscovery by address range

`qos_class`
:   QoS performance class cookie

`ops`
:   CXL root decoder operations

`cxlsd`
:   base cxl switch decoder

struct cxl\_region\_params
:   region settings

**Definition**:

```
struct cxl_region_params {
    enum cxl_config_state state;
    uuid_t uuid;
    int interleave_ways;
    int interleave_granularity;
    struct resource *res;
    struct cxl_endpoint_decoder *targets[CXL_DECODER_MAX_INTERLEAVE];
    int nr_targets;
    resource_size_t cache_size;
};
```

**Members**

`state`
:   allow the driver to lockdown further parameter changes

`uuid`
:   unique id for persistent regions

`interleave_ways`
:   number of endpoints in the region

`interleave_granularity`
:   capacity each endpoint contributes to a stripe

`res`
:   allocated iomem capacity for this region

`targets`
:   active ordered targets in current decoder configuration

`nr_targets`
:   number of targets

`cache_size`
:   extended linear cache size if exists, otherwise zero.

**Description**

State transitions are protected by cxl\_rwsem.region

struct cxl\_region
:   CXL region

**Definition**:

```
struct cxl_region {
    struct device dev;
    int id;
    struct cxl_root_decoder *cxlrd;
    struct range hpa_range;
    enum cxl_partition_mode mode;
    enum cxl_decoder_type type;
    struct cxl_nvdimm_bridge *cxl_nvb;
    struct cxl_pmem_region *cxlr_pmem;
    unsigned long flags;
    struct cxl_region_params params;
    struct access_coordinate coord[ACCESS_COORDINATE_MAX];
    struct notifier_block node_notifier;
    struct notifier_block adist_notifier;
};
```

**Members**

`dev`
:   This region’s device

`id`
:   This region’s id. Id is globally unique across all regions

`cxlrd`
:   Region’s root decoder

`hpa_range`
:   Address range occupied by the region

`mode`
:   Operational mode of the mapped capacity

`type`
:   Endpoint decoder target type

`cxl_nvb`
:   nvdimm bridge for coordinating **cxlr\_pmem** setup / shutdown

`cxlr_pmem`
:   (for pmem regions) cached copy of the nvdimm bridge

`flags`
:   Region state flags

`params`
:   active + config params for the region

`coord`
:   QoS access coordinates for the region

`node_notifier`
:   notifier for setting the access coordinates to node

`adist_notifier`
:   notifier for calculating the abstract distance of node

struct cxl\_port
:   logical collection of upstream port devices and downstream port devices to construct a CXL memory decode hierarchy.

**Definition**:

```
struct cxl_port {
    struct device dev;
    struct device *uport_dev;
    struct device *host_bridge;
    int id;
    struct xarray dports;
    struct xarray endpoints;
    struct xarray regions;
    struct cxl_dport *parent_dport;
    struct ida decoder_ida;
    struct cxl_register_map reg_map;
    struct cxl_component_regs regs;
    int nr_dports;
    int hdm_end;
    int commit_end;
    bool dead;
    unsigned int depth;
    struct cxl_cdat {
        void *table;
        size_t length;
    } cdat;
    bool cdat_available;
    long pci_latency;
    resource_size_t component_reg_phys;
};
```

**Members**

`dev`
:   this port’s device

`uport_dev`
:   PCI or platform device implementing the upstream port capability

`host_bridge`
:   Shortcut to the platform attach point for this port

`id`
:   id for port device-name

`dports`
:   cxl\_dport instances referenced by decoders

`endpoints`
:   cxl\_ep instances, endpoints that are a descendant of this port

`regions`
:   cxl\_region\_ref instances, regions mapped by this port

`parent_dport`
:   dport that points to this port in the parent

`decoder_ida`
:   allocator for decoder ids

`reg_map`
:   component and ras register mapping parameters

`regs`
:   mapped component registers

`nr_dports`
:   number of entries in **dports**

`hdm_end`
:   track last allocated HDM decoder instance for allocation ordering

`commit_end`
:   cursor to track highest committed decoder for commit ordering

`dead`
:   last ep has been removed, force port re-creation

`depth`
:   How deep this port is relative to the root. depth 0 is the root.

`cdat`
:   Cached CDAT data

`cdat_available`
:   Should a CDAT attribute be available in sysfs

`pci_latency`
:   Upstream latency in picoseconds

`component_reg_phys`
:   Physical address of component register

struct cxl\_root
:   logical collection of root cxl\_port items

**Definition**:

```
struct cxl_root {
    struct cxl_port port;
    struct cxl_root_ops ops;
};
```

**Members**

`port`
:   cxl\_port member

`ops`
:   cxl root operations

struct cxl\_dport
:   CXL downstream port

**Definition**:

```
struct cxl_dport {
    struct device *dport_dev;
    struct cxl_register_map reg_map;
    int port_id;
    struct cxl_rcrb_info rcrb;
    bool rch;
    struct cxl_port *port;
    struct cxl_regs regs;
    struct access_coordinate coord[ACCESS_COORDINATE_MAX];
    long link_latency;
    int gpf_dvsec;
};
```

**Members**

`dport_dev`
:   PCI bridge or firmware device representing the downstream link

`reg_map`
:   component and ras register mapping parameters

`port_id`
:   unique hardware identifier for dport in decoder target list

`rcrb`
:   Data about the Root Complex Register Block layout

`rch`
:   Indicate whether this dport was enumerated in RCH or VH mode

`port`
:   reference to cxl\_port that contains this downstream port

`regs`
:   Dport parsed register blocks

`coord`
:   access coordinates (bandwidth and latency performance attributes)

`link_latency`
:   calculated PCIe downstream latency

`gpf_dvsec`
:   Cached GPF port DVSEC

struct cxl\_ep
:   track an endpoint’s interest in a port

**Definition**:

```
struct cxl_ep {
    struct device *ep;
    struct cxl_dport *dport;
    struct cxl_port *next;
};
```

**Members**

`ep`
:   device that hosts a generic CXL endpoint (expander or accelerator)

`dport`
:   which dport routes to this endpoint on **port**

`next`
:   cxl switch port across the link attached to **dport** NULL if
    attached to an endpoint

struct cxl\_region\_ref
:   track a region’s interest in a port

**Definition**:

```
struct cxl_region_ref {
    struct cxl_port *port;
    struct cxl_decoder *decoder;
    struct cxl_region *region;
    struct xarray endpoints;
    int nr_targets_set;
    int nr_eps;
    int nr_targets;
};
```

**Members**

`port`
:   point in topology to install this reference

`decoder`
:   decoder assigned for **region** in **port**

`region`
:   region for this reference

`endpoints`
:   cxl\_ep references for region members beneath **port**

`nr_targets_set`
:   track how many targets have been programmed during setup

`nr_eps`
:   number of endpoints beneath **port**

`nr_targets`
:   number of distinct targets needed to reach **nr\_eps**

struct cxl\_endpoint\_dvsec\_info
:   Cached DVSEC info

**Definition**:

```
struct cxl_endpoint_dvsec_info {
    bool mem_enabled;
    int ranges;
    struct cxl_port *port;
    struct range dvsec_range[2];
};
```

**Members**

`mem_enabled`
:   cached value of mem\_enabled in the DVSEC at init time

`ranges`
:   Number of active HDM ranges this device uses.

`port`
:   endpoint port associated with this info instance

`dvsec_range`
:   cached attributes of the ranges in the DVSEC, PCIE\_DEVICE

int add\_cxl\_resources(struct resource \*cxl\_res)
:   reflect CXL fixed memory windows in iomem\_resource

**Parameters**

`struct resource *cxl_res`
:   A standalone resource tree where each CXL window is a sibling

**Description**

Walk each CXL window in **cxl\_res** and add it to iomem\_resource potentially
expanding its boundaries to ensure that any conflicting resources become
children. If a window is expanded it may then conflict with a another window
entry and require the window to be truncated or trimmed. Consider this
situation:

```
|-- "CXL Window 0" --||----- "CXL Window 1" -----|
|--------------- "System RAM" -------------|
```

...where platform firmware has established as System RAM resource across 2
windows, but has left some portion of window 1 for dynamic CXL region
provisioning. In this case “Window 0” will span the entirety of the “System
RAM” span, and “CXL Window 1” is truncated to the remaining tail past the end
of that “System RAM” resource.

Compute Express Link Host Managed Device Memory, starting with the
CXL 2.0 specification, is managed by an array of HDM Decoder register
instances per CXL port and per CXL endpoint. Define common helpers
for enumerating these registers and capabilities.

struct [cxl\_hdm](#c.cxl_hdm "cxl_hdm") \*devm\_cxl\_setup\_hdm(struct [cxl\_port](#c.cxl_port "cxl_port") \*port, struct [cxl\_endpoint\_dvsec\_info](#c.cxl_endpoint_dvsec_info "cxl_endpoint_dvsec_info") \*info)
:   map HDM decoder component registers

**Parameters**

`struct cxl_port *port`
:   cxl\_port to map

`struct cxl_endpoint_dvsec_info *info`
:   cached DVSEC range register info

int request\_skip(struct cxl\_dev\_state \*cxlds, struct [cxl\_endpoint\_decoder](#c.cxl_endpoint_decoder "cxl_endpoint_decoder") \*cxled, const resource\_size\_t skip\_base, const resource\_size\_t skip\_len)
:   Track DPA ‘skip’ in **cxlds->dpa\_res** resource tree

**Parameters**

`struct cxl_dev_state *cxlds`
:   CXL.mem device context that parents **cxled**

`struct cxl_endpoint_decoder *cxled`
:   Endpoint decoder establishing new allocation that skips lower DPA

`const resource_size_t skip_base`
:   DPA < start of new DPA allocation (DPAnew)

`const resource_size_t skip_len`
:   **skip\_base** + **skip\_len** == DPAnew

**Description**

DPA ‘skip’ arises from out-of-sequence DPA allocation events relative
to free capacity across multiple partitions. It is a wasteful event
as usable DPA gets thrown away, but if a deployment has, for example,
a dual RAM+PMEM device, wants to use PMEM, and has unallocated RAM
DPA, the free RAM DPA must be sacrificed to start allocating PMEM.
See third “Implementation Note” in CXL 3.1 8.2.4.19.13 “Decoder
Protection” for more details.

A ‘skip’ always covers the last allocated DPA in a previous partition
to the start of the current partition to allocate. Allocations never
start in the middle of a partition, and allocations are always
de-allocated in reverse order (see `cxl_dpa_free()`, or natural devm
unwind order from forced in-order allocation).

If **cxlds->nr\_partitions** was guaranteed to be <= 2 then the ‘skip’
would always be contained to a single partition. Given
**cxlds->nr\_partitions** may be > 2 it results in cases where the ‘skip’
might span “tail capacity of partition[0], all of partition[1], ...,
all of partition[N-1]” to support allocating from partition[N]. That
in turn interacts with the partition ‘`struct resource`’ boundaries
within **cxlds->dpa\_res** whereby ‘skip’ requests need to be divided by
partition. I.e. this is a quirk of using a ‘`struct resource`’ tree to
detect range conflicts while also tracking partition boundaries in
**cxlds->dpa\_res**.

int devm\_cxl\_enumerate\_decoders(struct [cxl\_hdm](#c.cxl_hdm "cxl_hdm") \*cxlhdm, struct [cxl\_endpoint\_dvsec\_info](#c.cxl_endpoint_dvsec_info "cxl_endpoint_dvsec_info") \*info)
:   add decoder objects per HDM register set

**Parameters**

`struct cxl_hdm *cxlhdm`
:   Structure to populate with HDM capabilities

`struct cxl_endpoint_dvsec_info *info`
:   cached DVSEC range register info

int devm\_cxl\_switch\_port\_decoders\_setup(struct [cxl\_port](#c.cxl_port "cxl_port") \*port)
:   allocate and setup switch decoders

**Parameters**

`struct cxl_port *port`
:   CXL port context

**Description**

Return 0 or -errno on error

int devm\_cxl\_endpoint\_decoders\_setup(struct [cxl\_port](#c.cxl_port "cxl_port") \*port)
:   allocate and setup endpoint decoders

**Parameters**

`struct cxl_port *port`
:   CXL port context

**Description**

Return 0 or -errno on error

void cxl\_coordinates\_combine(struct access\_coordinate \*out, struct access\_coordinate \*c1, struct access\_coordinate \*c2)
:   Combine the two input coordinates

**Parameters**

`struct access_coordinate *out`
:   Output coordinate of c1 and c2 combined

`struct access_coordinate *c1`
:   input coordinates

`struct access_coordinate *c2`
:   input coordinates

int cxl\_endpoint\_gather\_bandwidth(struct [cxl\_region](#c.cxl_region "cxl_region") \*cxlr, struct [cxl\_endpoint\_decoder](#c.cxl_endpoint_decoder "cxl_endpoint_decoder") \*cxled, struct [xarray](../../core-api/xarray.html#c.xarray "xarray") \*usp\_xa, bool \*gp\_is\_root)
:   collect all the endpoint bandwidth in an xarray

**Parameters**

`struct cxl_region *cxlr`
:   CXL region for the bandwidth calculation

`struct cxl_endpoint_decoder *cxled`
:   endpoint decoder to start on

`struct xarray *usp_xa`
:   (output) the xarray that collects all the bandwidth coordinates
    indexed by the upstream device with data of ‘`struct cxl_perf_ctx`’.

`bool *gp_is_root`
:   (output) bool of whether the grandparent is cxl root.

**Return**

0 for success or -errno

**Description**

Collects aggregated endpoint bandwidth and store the bandwidth in
an xarray indexed by the upstream device of the switch or the RP
device. Each endpoint consists the minimum of the bandwidth from DSLBIS
from the endpoint CDAT, the endpoint upstream link bandwidth, and the
bandwidth from the SSLBIS of the switch CDAT for the switch upstream port to
the downstream port that’s associated with the endpoint. If the
device is directly connected to a RP, then no SSLBIS is involved.

struct [xarray](../../core-api/xarray.html#c.xarray "xarray") \*cxl\_switch\_gather\_bandwidth(struct [cxl\_region](#c.cxl_region "cxl_region") \*cxlr, struct [xarray](../../core-api/xarray.html#c.xarray "xarray") \*input\_xa, bool \*gp\_is\_root)
:   collect all the bandwidth at switch level in an xarray

**Parameters**

`struct cxl_region *cxlr`
:   The region being operated on

`struct xarray *input_xa`
:   xarray indexed by upstream device of a switch with data of ‘`struct
    cxl_perf_ctx`’

`bool *gp_is_root`
:   (output) bool of whether the grandparent is cxl root.

**Return**

a xarray of resulting cxl\_perf\_ctx per parent switch or root port
or ERR\_PTR(-errno)

**Description**

Iterate through the xarray. Take the minimum of the downstream calculated
bandwidth, the upstream link bandwidth, and the SSLBIS of the upstream
switch if exists. Sum the resulting bandwidth under the switch upstream
device or a RP device. The function can be iterated over multiple switches
if the switches are present.

struct [xarray](../../core-api/xarray.html#c.xarray "xarray") \*cxl\_rp\_gather\_bandwidth(struct [xarray](../../core-api/xarray.html#c.xarray "xarray") \*xa)
:   handle the root port level bandwidth collection

**Parameters**

`struct xarray *xa`
:   the xarray that holds the cxl\_perf\_ctx that has the bandwidth calculated
    below each root port device.

**Return**

xarray that holds cxl\_perf\_ctx per host bridge or ERR\_PTR(-errno)

struct [xarray](../../core-api/xarray.html#c.xarray "xarray") \*cxl\_hb\_gather\_bandwidth(struct [xarray](../../core-api/xarray.html#c.xarray "xarray") \*xa)
:   handle the host bridge level bandwidth collection

**Parameters**

`struct xarray *xa`
:   the xarray that holds the cxl\_perf\_ctx that has the bandwidth calculated
    below each host bridge.

**Return**

xarray that holds cxl\_perf\_ctx per ACPI0017 device or ERR\_PTR(-errno)

void cxl\_region\_update\_bandwidth(struct [cxl\_region](#c.cxl_region "cxl_region") \*cxlr, struct [xarray](../../core-api/xarray.html#c.xarray "xarray") \*input\_xa)
:   Update the bandwidth access coordinates of a region

**Parameters**

`struct cxl_region *cxlr`
:   The region being operated on

`struct xarray *input_xa`
:   xarray holds cxl\_perf\_ctx with calculated bandwidth per ACPI0017 instance

void cxl\_region\_shared\_upstream\_bandwidth\_update(struct [cxl\_region](#c.cxl_region "cxl_region") \*cxlr)
:   Recalculate the bandwidth for the region

**Parameters**

`struct cxl_region *cxlr`
:   the cxl region to recalculate

**Description**

The function walks the topology from bottom up and calculates the bandwidth. It
starts at the endpoints, processes at the switches if any, processes at the rootport
level, at the host bridge level, and finally aggregates at the region.

The CXL core provides a set of interfaces that can be consumed by CXL aware
drivers. The interfaces allow for creation, modification, and destruction of
regions, memory devices, ports, and decoders. CXL aware drivers must register
with the CXL core via these interfaces in order to be able to participate in
cross-device interleave coordination. The CXL core also establishes and
maintains the bridge to the nvdimm subsystem.

CXL core introduces sysfs hierarchy to control the devices that are
instantiated by the core.

struct [cxl\_port](#c.cxl_port "cxl_port") \*devm\_cxl\_add\_port(struct [device](../infrastructure.html#c.device "device") \*host, struct [device](../infrastructure.html#c.device "device") \*uport\_dev, resource\_size\_t component\_reg\_phys, struct [cxl\_dport](#c.cxl_dport "cxl_dport") \*parent\_dport)
:   register a cxl\_port in CXL memory decode hierarchy

**Parameters**

`struct device *host`
:   host device for devm operations

`struct device *uport_dev`
:   “physical” device implementing this upstream port

`resource_size_t component_reg_phys`
:   (optional) for configurable cxl\_port instances

`struct cxl_dport *parent_dport`
:   next hop up in the CXL memory decode hierarchy

struct [cxl\_dport](#c.cxl_dport "cxl_dport") \*devm\_cxl\_add\_dport(struct [cxl\_port](#c.cxl_port "cxl_port") \*port, struct [device](../infrastructure.html#c.device "device") \*dport\_dev, int port\_id, resource\_size\_t component\_reg\_phys)
:   append VH downstream port data to a cxl\_port

**Parameters**

`struct cxl_port *port`
:   the cxl\_port that references this dport

`struct device *dport_dev`
:   firmware or PCI device representing the dport

`int port_id`
:   identifier for this dport in a decoder’s target list

`resource_size_t component_reg_phys`
:   optional location of CXL component registers

**Description**

Note that dports are appended to the devm release action’s of the
either the port’s host (for root ports), or the port itself (for
switch ports)

struct [cxl\_dport](#c.cxl_dport "cxl_dport") \*devm\_cxl\_add\_rch\_dport(struct [cxl\_port](#c.cxl_port "cxl_port") \*port, struct [device](../infrastructure.html#c.device "device") \*dport\_dev, int port\_id, resource\_size\_t rcrb)
:   append RCH downstream port data to a cxl\_port

**Parameters**

`struct cxl_port *port`
:   the cxl\_port that references this dport

`struct device *dport_dev`
:   firmware or PCI device representing the dport

`int port_id`
:   identifier for this dport in a decoder’s target list

`resource_size_t rcrb`
:   mandatory location of a Root Complex Register Block

**Description**

See CXL 3.0 9.11.8 CXL Devices Attached to an RCH

int cxl\_add\_ep(struct [cxl\_dport](#c.cxl_dport "cxl_dport") \*dport, struct [device](../infrastructure.html#c.device "device") \*ep\_dev)
:   register an endpoint’s interest in a port

**Parameters**

`struct cxl_dport *dport`
:   the dport that routes to **ep\_dev**

`struct device *ep_dev`
:   device representing the endpoint

**Description**

Intermediate CXL ports are scanned based on the arrival of endpoints.
When those endpoints depart the port can be destroyed once all
endpoints that care about that port have been removed.

struct [cxl\_port](#c.cxl_port "cxl_port") \*find\_cxl\_port\_by\_uport(struct [device](../infrastructure.html#c.device "device") \*uport\_dev)
:   Find a CXL port device companion

**Parameters**

`struct device *uport_dev`
:   Device that acts as a switch or endpoint in the CXL hierarchy

**Description**

In the case of endpoint ports recall that port->uport\_dev points to a ‘[`struct
cxl_memdev`](#c.cxl_memdev "cxl_memdev")’ device. So, the **uport\_dev** argument is the parent device of the
‘[`struct cxl_memdev`](#c.cxl_memdev "cxl_memdev")’ in that case.

Function takes a device reference on the port device. Caller should do a
[`put_device()`](../infrastructure.html#c.put_device "put_device") when done.

int cxl\_decoder\_init(struct [cxl\_port](#c.cxl_port "cxl_port") \*port, struct [cxl\_decoder](#c.cxl_decoder "cxl_decoder") \*cxld)
:   Common decoder setup / initialization

**Parameters**

`struct cxl_port *port`
:   owning port of this decoder

`struct cxl_decoder *cxld`
:   common decoder properties to initialize

**Description**

A port may contain one or more decoders. Each of those decoders
enable some address space for CXL.mem utilization. A decoder is
expected to be configured by the caller before registering via
[`cxl_decoder_add()`](#c.cxl_decoder_add "cxl_decoder_add")

struct [cxl\_root\_decoder](#c.cxl_root_decoder "cxl_root_decoder") \*cxl\_root\_decoder\_alloc(struct [cxl\_port](#c.cxl_port "cxl_port") \*port, unsigned int nr\_targets)
:   Allocate a root level decoder

**Parameters**

`struct cxl_port *port`
:   owning CXL root of this decoder

`unsigned int nr_targets`
:   static number of downstream targets

**Return**

A new cxl decoder to be registered by [`cxl_decoder_add()`](#c.cxl_decoder_add "cxl_decoder_add"). A
‘CXL root’ decoder is one that decodes from a top-level / static platform
firmware description of CXL resources into a CXL standard decode
topology.

struct [cxl\_switch\_decoder](#c.cxl_switch_decoder "cxl_switch_decoder") \*cxl\_switch\_decoder\_alloc(struct [cxl\_port](#c.cxl_port "cxl_port") \*port, unsigned int nr\_targets)
:   Allocate a switch level decoder

**Parameters**

`struct cxl_port *port`
:   owning CXL switch port of this decoder

`unsigned int nr_targets`
:   max number of dynamically addressable downstream targets

**Return**

A new cxl decoder to be registered by [`cxl_decoder_add()`](#c.cxl_decoder_add "cxl_decoder_add"). A
‘switch’ decoder is any decoder that can be enumerated by PCIe
topology and the HDM Decoder Capability. This includes the decoders
that sit between Switch Upstream Ports / Switch Downstream Ports and
Host Bridges / Root Ports.

struct [cxl\_endpoint\_decoder](#c.cxl_endpoint_decoder "cxl_endpoint_decoder") \*cxl\_endpoint\_decoder\_alloc(struct [cxl\_port](#c.cxl_port "cxl_port") \*port)
:   Allocate an endpoint decoder

**Parameters**

`struct cxl_port *port`
:   owning port of this decoder

**Return**

A new cxl decoder to be registered by [`cxl_decoder_add()`](#c.cxl_decoder_add "cxl_decoder_add")

int cxl\_decoder\_add\_locked(struct [cxl\_decoder](#c.cxl_decoder "cxl_decoder") \*cxld)
:   Add a decoder with targets

**Parameters**

`struct cxl_decoder *cxld`
:   The cxl decoder allocated by cxl\_<type>`_decoder_alloc()`

**Description**

Certain types of decoders may not have any targets. The main example of this
is an endpoint device. A more awkward example is a hostbridge whose root
ports get hot added (technically possible, though unlikely).

This is the locked variant of [`cxl_decoder_add()`](#c.cxl_decoder_add "cxl_decoder_add").

**Context**

Process context. Expects the device lock of the port that owns the
**cxld** to be held.

**Return**

Negative error code if the decoder wasn’t properly configured; else
returns 0.

int cxl\_decoder\_add(struct [cxl\_decoder](#c.cxl_decoder "cxl_decoder") \*cxld)
:   Add a decoder with targets

**Parameters**

`struct cxl_decoder *cxld`
:   The cxl decoder allocated by cxl\_<type>`_decoder_alloc()`

**Description**

This is the unlocked variant of [`cxl_decoder_add_locked()`](#c.cxl_decoder_add_locked "cxl_decoder_add_locked").
See [`cxl_decoder_add_locked()`](#c.cxl_decoder_add_locked "cxl_decoder_add_locked").

**Context**

Process context. Takes and releases the device lock of the port that
owns the **cxld**.

int \_\_cxl\_driver\_register(struct cxl\_driver \*cxl\_drv, struct module \*owner, const char \*modname)
:   register a driver for the cxl bus

**Parameters**

`struct cxl_driver *cxl_drv`
:   cxl driver structure to attach

`struct module *owner`
:   owning module/driver

`const char *modname`
:   KBUILD\_MODNAME for parent driver

int cxl\_endpoint\_get\_perf\_coordinates(struct [cxl\_port](#c.cxl_port "cxl_port") \*port, struct access\_coordinate \*coord)
:   Retrieve performance numbers stored in dports of CXL path

**Parameters**

`struct cxl_port *port`
:   endpoint cxl\_port

`struct access_coordinate *coord`
:   output performance data

**Return**

errno on failure, 0 on success.

Compute Express Link protocols are layered on top of PCIe. CXL core provides
a set of helpers for CXL interactions which occur via PCIe.

struct [cxl\_dport](#c.cxl_dport "cxl_dport") \*devm\_cxl\_add\_dport\_by\_dev(struct [cxl\_port](#c.cxl_port "cxl_port") \*port, struct [device](../infrastructure.html#c.device "device") \*dport\_dev)
:   allocate a dport by dport device

**Parameters**

`struct cxl_port *port`
:   cxl\_port that hosts the dport

`struct device *dport_dev`
:   ‘[`struct device`](../infrastructure.html#c.device "device")’ of the dport

**Description**

Returns the allocated dport on success or [`ERR_PTR()`](../../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR") of -errno on error

int cxl\_hdm\_decode\_init(struct cxl\_dev\_state \*cxlds, struct [cxl\_hdm](#c.cxl_hdm "cxl_hdm") \*cxlhdm, struct [cxl\_endpoint\_dvsec\_info](#c.cxl_endpoint_dvsec_info "cxl_endpoint_dvsec_info") \*info)
:   Setup HDM decoding for the endpoint

**Parameters**

`struct cxl_dev_state *cxlds`
:   Device state

`struct cxl_hdm *cxlhdm`
:   Mapped HDM decoder Capability

`struct cxl_endpoint_dvsec_info *info`
:   Cached DVSEC range registers info

**Description**

Try to enable the endpoint’s HDM Decoder Capability

void read\_cdat\_data(struct [cxl\_port](#c.cxl_port "cxl_port") \*port)
:   Read the CDAT data on this port

**Parameters**

`struct cxl_port *port`
:   Port to read data from

**Description**

This call will sleep waiting for responses from the DOE mailbox.

long cxl\_pci\_get\_latency(struct pci\_dev \*pdev)
:   calculate the link latency for the PCIe link

**Parameters**

`struct pci_dev *pdev`
:   PCI device

**Return**

calculated latency or 0 for no latency

**Description**

CXL Memory Device SW Guide v1.0 2.11.4 Link latency calculation
Link latency = LinkPropagationLatency + FlitLatency + RetimerLatency
LinkProgationLatency is negligible, so 0 will be used
RetimerLatency is assumed to be negligible and 0 will be used
FlitLatency = FlitSize / LinkBandwidth
FlitSize is defined by spec. CXL rev3.0 4.2.1.
68B flit is used up to 32GT/s. >32GT/s, 256B flit size is used.
The FlitLatency is converted to picoseconds.

The core CXL PMEM infrastructure supports persistent memory
provisioning and serves as a bridge to the LIBNVDIMM subsystem. A CXL
‘bridge’ device is added at the root of a CXL device topology if
platform firmware advertises at least one persistent memory capable
CXL window. That root-level bridge corresponds to a LIBNVDIMM ‘bus’
device. Then for each cxl\_memdev in the CXL device topology a bridge
device is added to host a LIBNVDIMM dimm object. When these bridges
are registered native LIBNVDIMM uapis are translated to CXL
operations, for example, namespace label access commands.

struct cxl\_nvdimm\_bridge \*cxl\_find\_nvdimm\_bridge(struct [cxl\_port](#c.cxl_port "cxl_port") \*port)
:   find a bridge device relative to a port

**Parameters**

`struct cxl_port *port`
:   any descendant port of an nvdimm-bridge associated
    root-cxl-port

int devm\_cxl\_add\_nvdimm(struct [device](../infrastructure.html#c.device "device") \*host, struct [cxl\_port](#c.cxl_port "cxl_port") \*port, struct [cxl\_memdev](#c.cxl_memdev "cxl_memdev") \*cxlmd)
:   add a bridge between a cxl\_memdev and an nvdimm

**Parameters**

`struct device *host`
:   host device for devm operations

`struct cxl_port *port`
:   any port in the CXL topology to find the nvdimm-bridge device

`struct cxl_memdev *cxlmd`
:   parent of the to be created cxl\_nvdimm device

**Return**

0 on success negative error code on failure.

CXL device capabilities are enumerated by PCI DVSEC (Designated
Vendor-specific) and / or descriptors provided by platform firmware.
They can be defined as a set like the device and component registers
mandated by CXL Section 8.1.12.2 Memory Device PCIe Capabilities and
Extended Capabilities, or they can be individual capabilities
appended to bridged and endpoint devices.

Provide common infrastructure for enumerating and mapping these
discrete capabilities.

void cxl\_probe\_component\_regs(struct [device](../infrastructure.html#c.device "device") \*dev, void \_\_iomem \*base, struct cxl\_component\_reg\_map \*map)
:   Detect CXL Component register blocks

**Parameters**

`struct device *dev`
:   Host device of the **base** mapping

`void __iomem *base`
:   Mapping containing the HDM Decoder Capability Header

`struct cxl_component_reg_map *map`
:   Map object describing the register block information found

**Description**

See CXL 2.0 8.2.4 Component Register Layout and Definition
See CXL 2.0 8.2.5.5 CXL Device Register Interface

Probe for component register information and return it in map object.

void cxl\_probe\_device\_regs(struct [device](../infrastructure.html#c.device "device") \*dev, void \_\_iomem \*base, struct cxl\_device\_reg\_map \*map)
:   Detect CXL Device register blocks

**Parameters**

`struct device *dev`
:   Host device of the **base** mapping

`void __iomem *base`
:   Mapping of CXL 2.0 8.2.8 CXL Device Register Interface

`struct cxl_device_reg_map *map`
:   Map object describing the register block information found

**Description**

Probe for device register information and return it in map object.

int cxl\_find\_regblock\_instance(struct pci\_dev \*pdev, enum cxl\_regloc\_type type, struct cxl\_register\_map \*map, unsigned int index)
:   Locate a register block by type / index

**Parameters**

`struct pci_dev *pdev`
:   The CXL PCI device to enumerate.

`enum cxl_regloc_type type`
:   Register Block Indicator id

`struct cxl_register_map *map`
:   Enumeration output, clobbered on error

`unsigned int index`
:   Index into which particular instance of a regblock wanted in the
    order found in register locator DVSEC.

**Return**

0 if register block enumerated, negative error code otherwise

**Description**

A CXL DVSEC may point to one or more register blocks, search for them
by **type** and **index**.

int cxl\_find\_regblock(struct pci\_dev \*pdev, enum cxl\_regloc\_type type, struct cxl\_register\_map \*map)
:   Locate register blocks by type

**Parameters**

`struct pci_dev *pdev`
:   The CXL PCI device to enumerate.

`enum cxl_regloc_type type`
:   Register Block Indicator id

`struct cxl_register_map *map`
:   Enumeration output, clobbered on error

**Return**

0 if register block enumerated, negative error code otherwise

**Description**

A CXL DVSEC may point to one or more register blocks, search for them
by **type**.

int cxl\_count\_regblock(struct pci\_dev \*pdev, enum cxl\_regloc\_type type)
:   Count instances of a given regblock type.

**Parameters**

`struct pci_dev *pdev`
:   The CXL PCI device to enumerate.

`enum cxl_regloc_type type`
:   Register Block Indicator id

**Description**

Some regblocks may be repeated. Count how many instances.

**Return**

non-negative count of matching regblocks, negative error code otherwise.

Core implementation of the CXL 2.0 Type-3 Memory Device Mailbox. The
implementation is used by the cxl\_pci driver to initialize the device
and implement the cxl\_mem.h IOCTL UAPI. It also implements the
backend of the `cxl_pmem_ctl()` transport for LIBNVDIMM.

int cxl\_internal\_send\_cmd(struct cxl\_mailbox \*cxl\_mbox, struct cxl\_mbox\_cmd \*mbox\_cmd)
:   Kernel internal interface to send a mailbox command

**Parameters**

`struct cxl_mailbox *cxl_mbox`
:   CXL mailbox context

`struct cxl_mbox_cmd *mbox_cmd`
:   initialized command to execute

**Context**

Any context.

**Return**

* %>=0 - Number of bytes returned in **out**.
* `-E2BIG` - Payload is too large for hardware.
* `-EBUSY` - Couldn’t acquire exclusive mailbox access.
* `-EFAULT` - Hardware error occurred.
* `-ENXIO` - Command completed, but device reported an error.
* `-EIO` - Unexpected output size.

**Description**

Mailbox commands may execute successfully yet the device itself reported an
error. While this distinction can be useful for commands from userspace, the
kernel will only be able to use results when both are successful.

bool cxl\_payload\_from\_user\_allowed(u16 opcode, void \*payload\_in, size\_t in\_size)
:   Check contents of in\_payload.

**Parameters**

`u16 opcode`
:   The mailbox command opcode.

`void *payload_in`
:   Pointer to the input payload passed in from user space.

`size_t in_size`
:   Size of **payload\_in** in bytes.

**Return**

* true - payload\_in passes check for **opcode**.
* false - payload\_in contains invalid or unsupported values.

**Description**

The driver may inspect payload contents before sending a mailbox
command from user space to the device. The intent is to reject
commands with input payloads that are known to be unsafe. This
check is not intended to replace the users careful selection of
mailbox command parameters and makes no guarantee that the user
command will succeed, nor that it is appropriate.

The specific checks are determined by the opcode.

int cxl\_validate\_cmd\_from\_user(struct cxl\_mbox\_cmd \*mbox\_cmd, struct cxl\_mailbox \*cxl\_mbox, const struct [cxl\_send\_command](#c.cxl_send_command "cxl_send_command") \*send\_cmd)
:   Check fields for CXL\_MEM\_SEND\_COMMAND.

**Parameters**

`struct cxl_mbox_cmd *mbox_cmd`
:   Sanitized and populated `struct cxl_mbox_cmd`.

`struct cxl_mailbox *cxl_mbox`
:   CXL mailbox context

`const struct cxl_send_command *send_cmd`
:   [`struct cxl_send_command`](#c.cxl_send_command "cxl_send_command") copied in from userspace.

**Return**

* `0` - **out\_cmd** is ready to send.
* `-ENOTTY` - Invalid command specified.
* `-EINVAL` - Reserved fields or invalid values were used.
* `-ENOMEM` - Input or output buffer wasn’t sized properly.
* `-EPERM` - Attempted to use a protected command.
* `-EBUSY` - Kernel has claimed exclusive access to this opcode

**Description**

The result of this command is a fully validated command in **mbox\_cmd** that is
safe to send to the hardware.

int handle\_mailbox\_cmd\_from\_user(struct cxl\_mailbox \*cxl\_mbox, struct cxl\_mbox\_cmd \*mbox\_cmd, u64 out\_payload, s32 \*size\_out, u32 \*retval)
:   Dispatch a mailbox command for userspace.

**Parameters**

`struct cxl_mailbox *cxl_mbox`
:   The mailbox context for the operation.

`struct cxl_mbox_cmd *mbox_cmd`
:   The validated mailbox command.

`u64 out_payload`
:   Pointer to userspace’s output payload.

`s32 *size_out`
:   (Input) Max payload size to copy out.
    (Output) Payload size hardware generated.

`u32 *retval`
:   Hardware generated return code from the operation.

**Return**

* `0` - Mailbox transaction succeeded. This implies the mailbox
  :   protocol completed successfully not that the operation itself
      was successful.
* `-ENOMEM` - Couldn’t allocate a bounce buffer.
* `-EFAULT` - Something happened with copy\_to/from\_user.
* `-EINTR` - Mailbox acquisition interrupted.
* `-EXXX` - Transaction level failures.

**Description**

Dispatches a mailbox command on behalf of a userspace request.
The output payload is copied to userspace.

See `cxl_send_cmd()`.

void cxl\_walk\_cel(struct [cxl\_memdev\_state](#c.cxl_memdev_state "cxl_memdev_state") \*mds, size\_t size, u8 \*cel)
:   Walk through the Command Effects Log.

**Parameters**

`struct cxl_memdev_state *mds`
:   The driver data for the operation

`size_t size`
:   Length of the Command Effects Log.

`u8 *cel`
:   CEL

**Description**

Iterate over each entry in the CEL and determine if the driver supports the
command. If so, the command is enabled for the device and can be used later.

int cxl\_enumerate\_cmds(struct [cxl\_memdev\_state](#c.cxl_memdev_state "cxl_memdev_state") \*mds)
:   Enumerate commands for a device.

**Parameters**

`struct cxl_memdev_state *mds`
:   The driver data for the operation

**Description**

Returns 0 if enumerate completed successfully.

CXL devices have optional support for certain commands. This function will
determine the set of supported commands for the hardware and update the
enabled\_cmds bitmap in the **mds**.

void cxl\_mem\_get\_event\_records(struct [cxl\_memdev\_state](#c.cxl_memdev_state "cxl_memdev_state") \*mds, u32 status)
:   Get Event Records from the device

**Parameters**

`struct cxl_memdev_state *mds`
:   The driver data for the operation

`u32 status`
:   Event Status register value identifying which events are available.

**Description**

Retrieve all event records available on the device, report them as trace
events, and clear them.

See CXL rev 3.0 **8.2.9.2.2** Get Event Records
See CXL rev 3.0 **8.2.9.2.3** Clear Event Records

int cxl\_mem\_get\_partition\_info(struct [cxl\_memdev\_state](#c.cxl_memdev_state "cxl_memdev_state") \*mds)
:   Get partition info

**Parameters**

`struct cxl_memdev_state *mds`
:   The driver data for the operation

**Description**

Retrieve the current partition info for the device specified. The active
values are the current capacity in bytes. If not 0, the ‘next’ values are
the pending values, in bytes, which take affect on next cold reset.

See CXL **8.2.9.5.2.1** Get Partition Info

**Return**

0 if no error: or the result of the mailbox command.

int cxl\_dev\_state\_identify(struct [cxl\_memdev\_state](#c.cxl_memdev_state "cxl_memdev_state") \*mds)
:   Send the IDENTIFY command to the device.

**Parameters**

`struct cxl_memdev_state *mds`
:   The driver data for the operation

**Return**

0 if identify was executed successfully or media not ready.

**Description**

This will dispatch the identify command to the device and on success populate
structures to be exported to sysfs.

int cxl\_mem\_sanitize(struct [cxl\_memdev](#c.cxl_memdev "cxl_memdev") \*cxlmd, u16 cmd)
:   Send a sanitization command to the device.

**Parameters**

`struct cxl_memdev *cxlmd`
:   The device for the operation

`u16 cmd`
:   The specific sanitization command opcode

**Return**

0 if the command was executed successfully, regardless of
whether or not the actual security operation is done in the background,
such as for the Sanitize case.
Error return values can be the result of the mailbox command, -EINVAL
when security requirements are not met or invalid contexts, or -EBUSY
if the sanitize operation is already in flight.

**Description**

See CXL 3.0 **8.2.9.8.5.1** Sanitize and **8.2.9.8.5.2** Secure Erase.

CXL Features:
A CXL device that includes a mailbox supports commands that allows
listing, getting, and setting of optionally defined features such
as memory sparing or post package sparing. Vendors may define custom
features for the device.

See [`devm_cxl_setup_features()`](../../userspace-api/fwctl/fwctl-cxl.html#c.devm_cxl_setup_features "devm_cxl_setup_features") for API details.

### CXL Regions

CXL Regions represent mapped memory capacity in system physical address
space. Whereas the CXL Root Decoders identify the bounds of potential CXL
Memory ranges, Regions represent the active mapped capacity by the HDM
Decoder Capability structures throughout the Host Bridges, Switches, and
Endpoints in the topology.

Region configuration has ordering constraints. UUID may be set at any time
but is only visible for persistent regions.
1. Interleave granularity
2. Interleave size
3. Decoder targets

struct [cxl\_decoder](#c.cxl_decoder "cxl_decoder") \*cxl\_port\_pick\_region\_decoder(struct [cxl\_port](#c.cxl_port "cxl_port") \*port, struct [cxl\_endpoint\_decoder](#c.cxl_endpoint_decoder "cxl_endpoint_decoder") \*cxled, struct [cxl\_region](#c.cxl_region "cxl_region") \*cxlr)
:   assign or lookup a decoder for a region

**Parameters**

`struct cxl_port *port`
:   a port in the ancestry of the endpoint implied by **cxled**

`struct cxl_endpoint_decoder *cxled`
:   endpoint decoder to be, or currently, mapped by **port**

`struct cxl_region *cxlr`
:   region to establish, or validate, decode **port**

**Description**

In the region creation path [`cxl_port_pick_region_decoder()`](#c.cxl_port_pick_region_decoder "cxl_port_pick_region_decoder") is an
allocator to find a free port. In the region assembly path, it is
recalling the decoder that platform firmware picked for validation
purposes.

The result is recorded in a ‘[`struct cxl_region_ref`](#c.cxl_region_ref "cxl_region_ref")’ in **port**.

int cxl\_port\_attach\_region(struct [cxl\_port](#c.cxl_port "cxl_port") \*port, struct [cxl\_region](#c.cxl_region "cxl_region") \*cxlr, struct [cxl\_endpoint\_decoder](#c.cxl_endpoint_decoder "cxl_endpoint_decoder") \*cxled, int pos)
:   track a region’s interest in a port by endpoint

**Parameters**

`struct cxl_port *port`
:   port to add a new region reference ‘[`struct cxl_region_ref`](#c.cxl_region_ref "cxl_region_ref")’

`struct cxl_region *cxlr`
:   region to attach to **port**

`struct cxl_endpoint_decoder *cxled`
:   endpoint decoder used to create or further pin a region reference

`int pos`
:   interleave position of **cxled** in **cxlr**

**Description**

The attach event is an opportunity to validate CXL decode setup
constraints and record metadata needed for programming HDM decoders,
in particular decoder target lists.

The steps are:

* validate that there are no other regions with a higher HPA already
  associated with **port**
* establish a region reference if one is not already present

  + additionally allocate a decoder instance that will host **cxlr** on
    **port**
* pin the region reference by the endpoint
* account for how many entries in **port**’s target list are needed to
  cover all of the added endpoints.

int cxl\_calc\_interleave\_pos(struct [cxl\_endpoint\_decoder](#c.cxl_endpoint_decoder "cxl_endpoint_decoder") \*cxled, struct range \*hpa\_range)
:   calculate an endpoint position in a region

**Parameters**

`struct cxl_endpoint_decoder *cxled`
:   endpoint decoder member of given region

`struct range *hpa_range`
:   translated HPA range of the endpoint

**Description**

The endpoint position is calculated by traversing the topology from
the endpoint to the root decoder and iteratively applying this
calculation:

> position = position \* parent\_ways + parent\_pos;

...where **position** is inferred from switch and root decoder target lists.

**Return**

position >= 0 on success
-ENXIO on failure

struct [cxl\_region](#c.cxl_region "cxl_region") \*devm\_cxl\_add\_region(struct [cxl\_root\_decoder](#c.cxl_root_decoder "cxl_root_decoder") \*cxlrd, int id, enum cxl\_partition\_mode mode, enum cxl\_decoder\_type type)
:   Adds a region to a decoder

**Parameters**

`struct cxl_root_decoder *cxlrd`
:   root decoder

`int id`
:   memregion id to create, or `memregion_free()` on failure

`enum cxl_partition_mode mode`
:   mode for the endpoint decoders of this region

`enum cxl_decoder_type type`
:   select whether this is an expander or accelerator (type-2 or type-3)

**Description**

This is the second step of region initialization. Regions exist within an
address space which is mapped by a **cxlrd**.

**Return**

0 if the region was added to the **cxlrd**, else returns negative error
code. The region will be named “regionZ” where Z is the unique region number.

int cxl\_validate\_translation\_params(u8 eiw, u16 eig, int pos)

**Parameters**

`u8 eiw`
:   encoded interleave ways

`u16 eig`
:   encoded interleave granularity

`int pos`
:   position in interleave

**Description**

Callers pass CXL\_POS\_ZERO when no position parameter needs validating.

**Return**

0 on success, -EINVAL on first invalid parameter

## External Interfaces

### CXL IOCTL Interface

Not all of the commands that the driver supports are available for use by
userspace at all times. Userspace can check the result of the QUERY command
to determine the live set of commands. Alternatively, it can issue the
command and check for failure.

struct cxl\_command\_info
:   Command information returned from a query.

**Definition**:

```
struct cxl_command_info {
    __u32 id;
    __u32 flags;
#define CXL_MEM_COMMAND_FLAG_MASK               GENMASK(1, 0);
#define CXL_MEM_COMMAND_FLAG_ENABLED            BIT(0);
#define CXL_MEM_COMMAND_FLAG_EXCLUSIVE          BIT(1);
    __u32 size_in;
    __u32 size_out;
};
```

**Members**

`id`
:   ID number for the command.

`flags`
:   Flags that specify command behavior.

`size_in`
:   Expected input size, or ~0 if variable length.

`size_out`
:   Expected output size, or ~0 if variable length.

**Description**

> CXL\_MEM\_COMMAND\_FLAG\_USER\_ENABLED
>
> The given command id is supported by the driver and is supported by
> a related opcode on the device.
>
> CXL\_MEM\_COMMAND\_FLAG\_EXCLUSIVE
>
> Requests with the given command id will terminate with EBUSY as the
> kernel actively owns management of the given resource. For example,
> the label-storage-area can not be written while the kernel is
> actively managing that space.

Represents a single command that is supported by both the driver and the
hardware. This is returned as part of an array from the query ioctl. The
following would be a command that takes a variable length input and returns 0
bytes of output.

> * **id** = 10
> * **flags** = CXL\_MEM\_COMMAND\_FLAG\_ENABLED
> * **size\_in** = ~0
> * **size\_out** = 0

See [`struct cxl_mem_query_commands`](#c.cxl_mem_query_commands "cxl_mem_query_commands").

struct cxl\_mem\_query\_commands
:   Query supported commands.

**Definition**:

```
struct cxl_mem_query_commands {
    __u32 n_commands;
    __u32 rsvd;
    struct cxl_command_info __user commands[];
};
```

**Members**

`n_commands`
:   In/out parameter. When **n\_commands** is > 0, the driver will
    return min(num\_support\_commands, n\_commands). When **n\_commands**
    is 0, driver will return the number of total supported commands.

`rsvd`
:   Reserved for future use.

`commands`
:   Output array of supported commands. This array must be allocated
    by userspace to be at least min(num\_support\_commands, **n\_commands**)

**Description**

Allow userspace to query the available commands supported by both the driver,
and the hardware. Commands that aren’t supported by either the driver, or the
hardware are not returned in the query.

**Examples**

> * { .n\_commands = 0 } // Get number of supported commands
> * { .n\_commands = 15, .commands = buf } // Return first 15 (or less)
>   supported commands
>
> See [`struct cxl_command_info`](#c.cxl_command_info "cxl_command_info").

struct cxl\_send\_command
:   Send a command to a memory device.

**Definition**:

```
struct cxl_send_command {
    __u32 id;
    __u32 flags;
    union {
        struct {
            __u16 opcode;
            __u16 rsvd;
        } raw;
        __u32 rsvd;
    };
    __u32 retval;
    struct {
        __u32 size;
        __u32 rsvd;
        __u64 payload;
    } in;
    struct {
        __u32 size;
        __u32 rsvd;
        __u64 payload;
    } out;
};
```

**Members**

`id`
:   The command to send to the memory device. This must be one of the
    commands returned by the query command.

`flags`
:   Flags for the command (input).

`{unnamed_union}`
:   anonymous

`raw`
:   Special fields for raw commands

`raw.opcode`
:   Opcode passed to hardware when using the RAW command.

`raw.rsvd`
:   Must be zero.

`rsvd`
:   Must be zero.

`retval`
:   Return value from the memory device (output).

`in`
:   Parameters associated with input payload.

`in.size`
:   Size of the payload to provide to the device (input).

`in.rsvd`
:   Must be zero.

`in.payload`
:   Pointer to memory for payload input, payload is little endian.

`out`
:   Parameters associated with output payload.

`out.size`
:   Size of the payload received from the device (input/output). This
    field is filled in by userspace to let the driver know how much
    space was allocated for output. It is populated by the driver to
    let userspace know how large the output payload actually was.

`out.rsvd`
:   Must be zero.

`out.payload`
:   Pointer to memory for payload output, payload is little endian.

**Description**

Mechanism for userspace to send a command to the hardware for processing. The
driver will do basic validation on the command sizes. In some cases even the
payload may be introspected. Userspace is required to allocate large enough
buffers for size\_out which can be variable length in certain situations.
