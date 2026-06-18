# IOMMUFD

> 출처(원문): https://docs.kernel.org/userspace-api/iommufd.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# IOMMUFD

Author:
:   Jason Gunthorpe

Author:
:   Kevin Tian

## Overview

IOMMUFD is the user API to control the IOMMU subsystem as it relates to managing
IO page tables from userspace using file descriptors. It intends to be general
and consumable by any driver that wants to expose DMA to userspace. These
drivers are eventually expected to deprecate any internal IOMMU logic
they may already/historically implement (e.g. vfio\_iommu\_type1.c).

At minimum iommufd provides universal support of managing I/O address spaces and
I/O page tables for all IOMMUs, with room in the design to add non-generic
features to cater to specific hardware functionality.

In this context the capital letter (IOMMUFD) refers to the subsystem while the
small letter (iommufd) refers to the file descriptors created via /dev/iommu for
use by userspace.

## Key Concepts

### User Visible Objects

Following IOMMUFD objects are exposed to userspace:

* IOMMUFD\_OBJ\_IOAS, representing an I/O address space (IOAS), allowing map/unmap
  of user space memory into ranges of I/O Virtual Address (IOVA).

  The IOAS is a functional replacement for the VFIO container, and like the VFIO
  container it copies an IOVA map to a list of iommu\_domains held within it.
* IOMMUFD\_OBJ\_DEVICE, representing a device that is bound to iommufd by an
  external driver.
* IOMMUFD\_OBJ\_HWPT\_PAGING, representing an actual hardware I/O page table
  (i.e. a single `struct iommu_domain`) managed by the iommu driver. “PAGING”
  primarily indicates this type of HWPT should be linked to an IOAS. It also
  indicates that it is backed by an iommu\_domain with \_\_IOMMU\_DOMAIN\_PAGING
  feature flag. This can be either an UNMANAGED stage-1 domain for a device
  running in the user space, or a nesting parent stage-2 domain for mappings
  from guest-level physical addresses to host-level physical addresses.

  The IOAS has a list of HWPT\_PAGINGs that share the same IOVA mapping and
  it will synchronize its mapping with each member HWPT\_PAGING.
* IOMMUFD\_OBJ\_HWPT\_NESTED, representing an actual hardware I/O page table
  (i.e. a single `struct iommu_domain`) managed by user space (e.g. guest OS).
  “NESTED” indicates that this type of HWPT should be linked to an HWPT\_PAGING.
  It also indicates that it is backed by an iommu\_domain that has a type of
  IOMMU\_DOMAIN\_NESTED. This must be a stage-1 domain for a device running in
  the user space (e.g. in a guest VM enabling the IOMMU nested translation
  feature.) As such, it must be created with a given nesting parent stage-2
  domain to associate to. This nested stage-1 page table managed by the user
  space usually has mappings from guest-level I/O virtual addresses to guest-
  level physical addresses.
* IOMMUFD\_FAULT, representing a software queue for an HWPT reporting IO page
  faults using the IOMMU HW’s PRI (Page Request Interface). This queue object
  provides user space an FD to poll the page fault events and also to respond
  to those events. A FAULT object must be created first to get a fault\_id that
  could be then used to allocate a fault-enabled HWPT via the IOMMU\_HWPT\_ALLOC
  command by setting the IOMMU\_HWPT\_FAULT\_ID\_VALID bit in its flags field.
* IOMMUFD\_OBJ\_VIOMMU, representing a slice of the physical IOMMU instance,
  passed to or shared with a VM. It may be some HW-accelerated virtualization
  features and some SW resources used by the VM. For examples:

  + Security namespace for guest owned ID, e.g. guest-controlled cache tags
  + Non-device-affiliated event reporting, e.g. invalidation queue errors
  + Access to a shareable nesting parent pagetable across physical IOMMUs
  + Virtualization of various platforms IDs, e.g. RIDs and others
  + Delivery of paravirtualized invalidation
  + Direct assigned invalidation queues
  + Direct assigned interrupts

  Such a vIOMMU object generally has the access to a nesting parent pagetable
  to support some HW-accelerated virtualization features. So, a vIOMMU object
  must be created given a nesting parent HWPT\_PAGING object, and then it would
  encapsulate that HWPT\_PAGING object. Therefore, a vIOMMU object can be used
  to allocate an HWPT\_NESTED object in place of the encapsulated HWPT\_PAGING.

  Note

  The name “vIOMMU” isn’t necessarily identical to a virtualized IOMMU in a
  VM. A VM can have one giant virtualized IOMMU running on a machine having
  multiple physical IOMMUs, in which case the VMM will dispatch the requests
  or configurations from this single virtualized IOMMU instance to multiple
  vIOMMU objects created for individual slices of different physical IOMMUs.
  In other words, a vIOMMU object is always a representation of one physical
  IOMMU, not necessarily of a virtualized IOMMU. For VMMs that want the full
  virtualization features from physical IOMMUs, it is suggested to build the
  same number of virtualized IOMMUs as the number of physical IOMMUs, so the
  passed-through devices would be connected to their own virtualized IOMMUs
  backed by corresponding vIOMMU objects, in which case a guest OS would do
  the “dispatch” naturally instead of VMM trappings.
* IOMMUFD\_OBJ\_VDEVICE, representing a virtual device for an IOMMUFD\_OBJ\_DEVICE
  against an IOMMUFD\_OBJ\_VIOMMU. This virtual device holds the device’s virtual
  information or attributes (related to the vIOMMU) in a VM. An immediate vDATA
  example can be the virtual ID of the device on a vIOMMU, which is a unique ID
  that VMM assigns to the device for a translation channel/port of the vIOMMU,
  e.g. vSID of ARM SMMUv3, vDeviceID of AMD IOMMU, and vRID of Intel VT-d to a
  Context Table. Potential use cases of some advanced security information can
  be forwarded via this object too, such as security level or realm information
  in a Confidential Compute Architecture. A VMM should create a vDEVICE object
  to forward all the device information in a VM, when it connects a device to a
  vIOMMU, which is a separate ioctl call from attaching the same device to an
  HWPT\_PAGING that the vIOMMU holds.
* IOMMUFD\_OBJ\_VEVENTQ, representing a software queue for a vIOMMU to report its
  events such as translation faults occurred to a nested stage-1 (excluding I/O
  page faults that should go through IOMMUFD\_OBJ\_FAULT) and HW-specific events.
  This queue object provides user space an FD to poll/read the vIOMMU events. A
  vIOMMU object must be created first to get its viommu\_id, which could be then
  used to allocate a vEVENTQ. Each vIOMMU can support multiple types of vEVENTS,
  but is confined to one vEVENTQ per vEVENTQ type.
* IOMMUFD\_OBJ\_HW\_QUEUE, representing a hardware accelerated queue, as a subset
  of IOMMU’s virtualization features, for the IOMMU HW to directly read or write
  the virtual queue memory owned by a guest OS. This HW-acceleration feature can
  allow VM to work with the IOMMU HW directly without a VM Exit, so as to reduce
  overhead from the hypercalls. Along with the HW QUEUE object, iommufd provides
  user space an mmap interface for VMM to mmap a physical MMIO region from the
  host physical address space to the guest physical address space, allowing the
  guest OS to directly control the allocated HW QUEUE. Thus, when allocating a
  HW QUEUE, the VMM must request a pair of mmap info (offset/length) and pass in
  exactly to an mmap syscall via its offset and length arguments.

All user-visible objects are destroyed via the IOMMU\_DESTROY uAPI.

The diagrams below show relationships between user-visible objects and kernel
datastructures (external to iommufd), with numbers referred to operations
creating the objects and links:

```
 _______________________________________________________________________
|                      iommufd (HWPT_PAGING only)                       |
|                                                                       |
|        [1]                  [3]                                [2]    |
|  ________________      _____________                        ________  |
| |                |    |             |                      |        | |
| |      IOAS      |<---| HWPT_PAGING |<---------------------| DEVICE | |
| |________________|    |_____________|                      |________| |
|         |                    |                                  |     |
|_________|____________________|__________________________________|_____|
          |                    |                                  |
          |              ______v_____                          ___v__
          | PFN storage |  (paging)  |                        |struct|
          |------------>|iommu_domain|<-----------------------|device|
                        |____________|                        |______|

 _______________________________________________________________________
|                      iommufd (with HWPT_NESTED)                       |
|                                                                       |
|        [1]                  [3]                [4]             [2]    |
|  ________________      _____________      _____________     ________  |
| |                |    |             |    |             |   |        | |
| |      IOAS      |<---| HWPT_PAGING |<---| HWPT_NESTED |<--| DEVICE | |
| |________________|    |_____________|    |_____________|   |________| |
|         |                    |                  |               |     |
|_________|____________________|__________________|_______________|_____|
          |                    |                  |               |
          |              ______v_____       ______v_____       ___v__
          | PFN storage |  (paging)  |     |  (nested)  |     |struct|
          |------------>|iommu_domain|<----|iommu_domain|<----|device|
                        |____________|     |____________|     |______|

 _______________________________________________________________________
|                      iommufd (with vIOMMU/vDEVICE)                    |
|                                                                       |
|                             [5]                [6]                    |
|                        _____________      _____________               |
|                       |             |    |             |              |
|      |----------------|    vIOMMU   |<---|   vDEVICE   |<----|        |
|      |                |             |    |_____________|     |        |
|      |                |             |                        |        |
|      |      [1]       |             |          [4]           | [2]    |
|      |     ______     |             |     _____________     _|______  |
|      |    |      |    |     [3]     |    |             |   |        | |
|      |    | IOAS |<---|(HWPT_PAGING)|<---| HWPT_NESTED |<--| DEVICE | |
|      |    |______|    |_____________|    |_____________|   |________| |
|      |        |              |                  |               |     |
|______|________|______________|__________________|_______________|_____|
       |        |              |                  |               |
 ______v_____   |        ______v_____       ______v_____       ___v__
|   struct   |  |  PFN  |  (paging)  |     |  (nested)  |     |struct|
|iommu_device|  |------>|iommu_domain|<----|iommu_domain|<----|device|
|____________|   storage|____________|     |____________|     |______|
```

1. IOMMUFD\_OBJ\_IOAS is created via the IOMMU\_IOAS\_ALLOC uAPI. An iommufd can
   hold multiple IOAS objects. IOAS is the most generic object and does not
   expose interfaces that are specific to single IOMMU drivers. All operations
   on the IOAS must operate equally on each of the iommu\_domains inside of it.
2. IOMMUFD\_OBJ\_DEVICE is created when an external driver calls the IOMMUFD kAPI
   to bind a device to an iommufd. The driver is expected to implement a set of
   ioctls to allow userspace to initiate the binding operation. Successful
   completion of this operation establishes the desired DMA ownership over the
   device. The driver must also set the driver\_managed\_dma flag and must not
   touch the device until this operation succeeds.
3. IOMMUFD\_OBJ\_HWPT\_PAGING can be created in two ways:

   * IOMMUFD\_OBJ\_HWPT\_PAGING is automatically created when an external driver
     calls the IOMMUFD kAPI to attach a bound device to an IOAS. Similarly the
     external driver uAPI allows userspace to initiate the attaching operation.
     If a compatible member HWPT\_PAGING object exists in the IOAS’s HWPT\_PAGING
     list, then it will be reused. Otherwise a new HWPT\_PAGING that represents
     an iommu\_domain to userspace will be created, and then added to the list.
     Successful completion of this operation sets up the linkages among IOAS,
     device and iommu\_domain. Once this completes the device could do DMA.
   * IOMMUFD\_OBJ\_HWPT\_PAGING can be manually created via the IOMMU\_HWPT\_ALLOC
     uAPI, provided an ioas\_id via @pt\_id to associate the new HWPT\_PAGING to
     the corresponding IOAS object. The benefit of this manual allocation is to
     allow allocation flags (defined in [`enum iommufd_hwpt_alloc_flags`](#c.iommufd_hwpt_alloc_flags "iommufd_hwpt_alloc_flags")), e.g. it
     allocates a nesting parent HWPT\_PAGING if the IOMMU\_HWPT\_ALLOC\_NEST\_PARENT
     flag is set.
4. IOMMUFD\_OBJ\_HWPT\_NESTED can be only manually created via the IOMMU\_HWPT\_ALLOC
   uAPI, provided an hwpt\_id or a viommu\_id of a vIOMMU object encapsulating a
   nesting parent HWPT\_PAGING via @pt\_id to associate the new HWPT\_NESTED object
   to the corresponding HWPT\_PAGING object. The associating HWPT\_PAGING object
   must be a nesting parent manually allocated via the same uAPI previously with
   an IOMMU\_HWPT\_ALLOC\_NEST\_PARENT flag, otherwise the allocation will fail. The
   allocation will be further validated by the IOMMU driver to ensure that the
   nesting parent domain and the nested domain being allocated are compatible.
   Successful completion of this operation sets up linkages among IOAS, device,
   and iommu\_domains. Once this completes the device could do DMA via a 2-stage
   translation, a.k.a nested translation. Note that multiple HWPT\_NESTED objects
   can be allocated by (and then associated to) the same nesting parent.

   Note

   Either a manual IOMMUFD\_OBJ\_HWPT\_PAGING or an IOMMUFD\_OBJ\_HWPT\_NESTED is
   created via the same IOMMU\_HWPT\_ALLOC uAPI. The difference is at the type
   of the object passed in via the @pt\_id field of `struct iommufd_hwpt_alloc`.
5. IOMMUFD\_OBJ\_VIOMMU can be only manually created via the IOMMU\_VIOMMU\_ALLOC
   uAPI, provided a dev\_id (for the device’s physical IOMMU to back the vIOMMU)
   and an hwpt\_id (to associate the vIOMMU to a nesting parent HWPT\_PAGING). The
   iommufd core will link the vIOMMU object to the `struct iommu_device` that the
   [`struct device`](../driver-api/infrastructure.html#c.device "device") is behind. And an IOMMU driver can implement a viommu\_alloc op
   to allocate its own vIOMMU data structure embedding the core-level structure
   iommufd\_viommu and some driver-specific data. If necessary, the driver can
   also configure its HW virtualization feature for that vIOMMU (and thus for
   the VM). Successful completion of this operation sets up the linkages between
   the vIOMMU object and the HWPT\_PAGING, then this vIOMMU object can be used
   as a nesting parent object to allocate an HWPT\_NESTED object described above.
6. IOMMUFD\_OBJ\_VDEVICE can be only manually created via the IOMMU\_VDEVICE\_ALLOC
   uAPI, provided a viommu\_id for an iommufd\_viommu object and a dev\_id for an
   iommufd\_device object. The vDEVICE object will be the binding between these
   two parent objects. Another @virt\_id will be also set via the uAPI providing
   the iommufd core an index to store the vDEVICE object to a vDEVICE array per
   vIOMMU. If necessary, the IOMMU driver may choose to implement a vdevce\_alloc
   op to init its HW for virtualization feature related to a vDEVICE. Successful
   completion of this operation sets up the linkages between vIOMMU and device.

A device can only bind to an iommufd due to DMA ownership claim and attach to at
most one IOAS object (no support of PASID yet).

### Kernel Datastructure

User visible objects are backed by following datastructures:

* iommufd\_ioas for IOMMUFD\_OBJ\_IOAS.
* iommufd\_device for IOMMUFD\_OBJ\_DEVICE.
* iommufd\_hwpt\_paging for IOMMUFD\_OBJ\_HWPT\_PAGING.
* iommufd\_hwpt\_nested for IOMMUFD\_OBJ\_HWPT\_NESTED.
* iommufd\_fault for IOMMUFD\_OBJ\_FAULT.
* iommufd\_viommu for IOMMUFD\_OBJ\_VIOMMU.
* iommufd\_vdevice for IOMMUFD\_OBJ\_VDEVICE.
* iommufd\_veventq for IOMMUFD\_OBJ\_VEVENTQ.
* iommufd\_hw\_queue for IOMMUFD\_OBJ\_HW\_QUEUE.

Several terminologies when looking at these datastructures:

* Automatic domain - refers to an iommu domain created automatically when
  attaching a device to an IOAS object. This is compatible to the semantics of
  VFIO type1.
* Manual domain - refers to an iommu domain designated by the user as the
  target pagetable to be attached to by a device. Though currently there are
  no uAPIs to directly create such domain, the datastructure and algorithms
  are ready for handling that use case.
* In-kernel user - refers to something like a VFIO mdev that is using the
  IOMMUFD access interface to access the IOAS. This starts by creating an
  iommufd\_access object that is similar to the domain binding a physical device
  would do. The access object will then allow converting IOVA ranges into `struct
  page` \* lists, or doing direct read/write to an IOVA.

iommufd\_ioas serves as the metadata datastructure to manage how IOVA ranges are
mapped to memory pages, composed of:

* `struct io_pagetable` holding the IOVA map
* `struct iopt_area`’s representing populated portions of IOVA
* `struct iopt_pages` representing the storage of PFNs
* `struct iommu_domain` representing the IO page table in the IOMMU
* `struct iopt_pages_access` representing in-kernel users of PFNs
* [`struct xarray`](../core-api/xarray.html#c.xarray "xarray") pinned\_pfns holding a list of pages pinned by in-kernel users

Each iopt\_pages represents a logical linear array of full PFNs. The PFNs are
ultimately derived from userspace VAs via an mm\_struct. Once they have been
pinned the PFNs are stored in IOPTEs of an iommu\_domain or inside the pinned\_pfns
xarray if they have been pinned through an iommufd\_access.

PFN have to be copied between all combinations of storage locations, depending
on what domains are present and what kinds of in-kernel “software access” users
exist. The mechanism ensures that a page is pinned only once.

An io\_pagetable is composed of iopt\_areas pointing at iopt\_pages, along with a
list of iommu\_domains that mirror the IOVA to PFN map.

Multiple io\_pagetable-s, through their iopt\_area-s, can share a single
iopt\_pages which avoids multi-pinning and double accounting of page
consumption.

iommufd\_ioas is shareable between subsystems, e.g. VFIO and VDPA, as long as
devices managed by different subsystems are bound to a same iommufd.

## IOMMUFD User API

**General ioctl format**

The ioctl interface follows a general format to allow for extensibility. Each
ioctl is passed in a structure pointer as the argument providing the size of
the structure in the first u32. The kernel checks that any structure space
beyond what it understands is 0. This allows userspace to use the backward
compatible portion while consistently using the newer, larger, structures.

ioctls use a standard meaning for common errnos:

> * ENOTTY: The IOCTL number itself is not supported at all
> * E2BIG: The IOCTL number is supported, but the provided structure has
>   non-zero in a part the kernel does not understand.
> * EOPNOTSUPP: The IOCTL number is supported, and the structure is
>   understood, however a known field has a value the kernel does not
>   understand or support.
> * EINVAL: Everything about the IOCTL was understood, but a field is not
>   correct.
> * ENOENT: An ID or IOVA provided does not exist.
> * ENOMEM: Out of memory.
> * EOVERFLOW: Mathematics overflowed.

As well as additional errnos, within specific ioctls.

struct iommu\_destroy
:   ioctl(IOMMU\_DESTROY)

**Definition**:

```
struct iommu_destroy {
    __u32 size;
    __u32 id;
};
```

**Members**

`size`
:   sizeof([`struct iommu_destroy`](#c.iommu_destroy "iommu_destroy"))

`id`
:   iommufd object ID to destroy. Can be any destroyable object type.

**Description**

Destroy any object held within iommufd.

struct iommu\_ioas\_alloc
:   ioctl(IOMMU\_IOAS\_ALLOC)

**Definition**:

```
struct iommu_ioas_alloc {
    __u32 size;
    __u32 flags;
    __u32 out_ioas_id;
};
```

**Members**

`size`
:   sizeof([`struct iommu_ioas_alloc`](#c.iommu_ioas_alloc "iommu_ioas_alloc"))

`flags`
:   Must be 0

`out_ioas_id`
:   Output IOAS ID for the allocated object

**Description**

Allocate an IO Address Space (IOAS) which holds an IO Virtual Address (IOVA)
to memory mapping.

struct iommu\_iova\_range
:   ioctl(IOMMU\_IOVA\_RANGE)

**Definition**:

```
struct iommu_iova_range {
    __aligned_u64 start;
    __aligned_u64 last;
};
```

**Members**

`start`
:   First IOVA

`last`
:   Inclusive last IOVA

**Description**

An interval in IOVA space.

struct iommu\_ioas\_iova\_ranges
:   ioctl(IOMMU\_IOAS\_IOVA\_RANGES)

**Definition**:

```
struct iommu_ioas_iova_ranges {
    __u32 size;
    __u32 ioas_id;
    __u32 num_iovas;
    __u32 __reserved;
    __aligned_u64 allowed_iovas;
    __aligned_u64 out_iova_alignment;
};
```

**Members**

`size`
:   sizeof([`struct iommu_ioas_iova_ranges`](#c.iommu_ioas_iova_ranges "iommu_ioas_iova_ranges"))

`ioas_id`
:   IOAS ID to read ranges from

`num_iovas`
:   Input/Output total number of ranges in the IOAS

`__reserved`
:   Must be 0

`allowed_iovas`
:   Pointer to the output array of [`struct iommu_iova_range`](#c.iommu_iova_range "iommu_iova_range")

`out_iova_alignment`
:   Minimum alignment required for mapping IOVA

**Description**

Query an IOAS for ranges of allowed IOVAs. Mapping IOVA outside these ranges
is not allowed. num\_iovas will be set to the total number of iovas and
the allowed\_iovas[] will be filled in as space permits.

The allowed ranges are dependent on the HW path the DMA operation takes, and
can change during the lifetime of the IOAS. A fresh empty IOAS will have a
full range, and each attached device will narrow the ranges based on that
device’s HW restrictions. Detaching a device can widen the ranges. Userspace
should query ranges after every attach/detach to know what IOVAs are valid
for mapping.

On input num\_iovas is the length of the allowed\_iovas array. On output it is
the total number of iovas filled in. The ioctl will return -EMSGSIZE and set
num\_iovas to the required value if num\_iovas is too small. In this case the
caller should allocate a larger output array and re-issue the ioctl.

out\_iova\_alignment returns the minimum IOVA alignment that can be given
to IOMMU\_IOAS\_MAP/COPY. IOVA’s must satisfy:

```
starting_iova % out_iova_alignment == 0
(starting_iova + length) % out_iova_alignment == 0
```

out\_iova\_alignment can be 1 indicating any IOVA is allowed. It cannot
be higher than the system PAGE\_SIZE.

struct iommu\_ioas\_allow\_iovas
:   ioctl(IOMMU\_IOAS\_ALLOW\_IOVAS)

**Definition**:

```
struct iommu_ioas_allow_iovas {
    __u32 size;
    __u32 ioas_id;
    __u32 num_iovas;
    __u32 __reserved;
    __aligned_u64 allowed_iovas;
};
```

**Members**

`size`
:   sizeof([`struct iommu_ioas_allow_iovas`](#c.iommu_ioas_allow_iovas "iommu_ioas_allow_iovas"))

`ioas_id`
:   IOAS ID to allow IOVAs from

`num_iovas`
:   Input/Output total number of ranges in the IOAS

`__reserved`
:   Must be 0

`allowed_iovas`
:   Pointer to array of [`struct iommu_iova_range`](#c.iommu_iova_range "iommu_iova_range")

**Description**

Ensure a range of IOVAs are always available for allocation. If this call
succeeds then IOMMU\_IOAS\_IOVA\_RANGES will never return a list of IOVA ranges
that are narrower than the ranges provided here. This call will fail if
IOMMU\_IOAS\_IOVA\_RANGES is currently narrower than the given ranges.

When an IOAS is first created the IOVA\_RANGES will be maximally sized, and as
devices are attached the IOVA will narrow based on the device restrictions.
When an allowed range is specified any narrowing will be refused, ie device
attachment can fail if the device requires limiting within the allowed range.

Automatic IOVA allocation is also impacted by this call. MAP will only
allocate within the allowed IOVAs if they are present.

This call replaces the entire allowed list with the given list.

enum iommufd\_ioas\_map\_flags
:   Flags for map and copy

**Constants**

`IOMMU_IOAS_MAP_FIXED_IOVA`
:   If clear the kernel will compute an appropriate
    IOVA to place the mapping at

`IOMMU_IOAS_MAP_WRITEABLE`
:   DMA is allowed to write to this mapping

`IOMMU_IOAS_MAP_READABLE`
:   DMA is allowed to read from this mapping

struct iommu\_ioas\_map
:   ioctl(IOMMU\_IOAS\_MAP)

**Definition**:

```
struct iommu_ioas_map {
    __u32 size;
    __u32 flags;
    __u32 ioas_id;
    __u32 __reserved;
    __aligned_u64 user_va;
    __aligned_u64 length;
    __aligned_u64 iova;
};
```

**Members**

`size`
:   sizeof([`struct iommu_ioas_map`](#c.iommu_ioas_map "iommu_ioas_map"))

`flags`
:   Combination of [`enum iommufd_ioas_map_flags`](#c.iommufd_ioas_map_flags "iommufd_ioas_map_flags")

`ioas_id`
:   IOAS ID to change the mapping of

`__reserved`
:   Must be 0

`user_va`
:   Userspace pointer to start mapping from

`length`
:   Number of bytes to map

`iova`
:   IOVA the mapping was placed at. If IOMMU\_IOAS\_MAP\_FIXED\_IOVA is set
    then this must be provided as input.

**Description**

Set an IOVA mapping from a user pointer. If FIXED\_IOVA is specified then the
mapping will be established at iova, otherwise a suitable location based on
the reserved and allowed lists will be automatically selected and returned in
iova.

If IOMMU\_IOAS\_MAP\_FIXED\_IOVA is specified then the iova range must currently
be unused, existing IOVA cannot be replaced.

struct iommu\_ioas\_map\_file
:   ioctl(IOMMU\_IOAS\_MAP\_FILE)

**Definition**:

```
struct iommu_ioas_map_file {
    __u32 size;
    __u32 flags;
    __u32 ioas_id;
    __s32 fd;
    __aligned_u64 start;
    __aligned_u64 length;
    __aligned_u64 iova;
};
```

**Members**

`size`
:   sizeof([`struct iommu_ioas_map_file`](#c.iommu_ioas_map_file "iommu_ioas_map_file"))

`flags`
:   same as for iommu\_ioas\_map

`ioas_id`
:   same as for iommu\_ioas\_map

`fd`
:   the memfd to map

`start`
:   byte offset from start of file to map from

`length`
:   same as for iommu\_ioas\_map

`iova`
:   same as for iommu\_ioas\_map

**Description**

Set an IOVA mapping from a memfd file. All other arguments and semantics
match those of IOMMU\_IOAS\_MAP.

struct iommu\_ioas\_copy
:   ioctl(IOMMU\_IOAS\_COPY)

**Definition**:

```
struct iommu_ioas_copy {
    __u32 size;
    __u32 flags;
    __u32 dst_ioas_id;
    __u32 src_ioas_id;
    __aligned_u64 length;
    __aligned_u64 dst_iova;
    __aligned_u64 src_iova;
};
```

**Members**

`size`
:   sizeof([`struct iommu_ioas_copy`](#c.iommu_ioas_copy "iommu_ioas_copy"))

`flags`
:   Combination of [`enum iommufd_ioas_map_flags`](#c.iommufd_ioas_map_flags "iommufd_ioas_map_flags")

`dst_ioas_id`
:   IOAS ID to change the mapping of

`src_ioas_id`
:   IOAS ID to copy from

`length`
:   Number of bytes to copy and map

`dst_iova`
:   IOVA the mapping was placed at. If IOMMU\_IOAS\_MAP\_FIXED\_IOVA is
    set then this must be provided as input.

`src_iova`
:   IOVA to start the copy

**Description**

Copy an already existing mapping from src\_ioas\_id and establish it in
dst\_ioas\_id. The src iova/length must exactly match a range used with
IOMMU\_IOAS\_MAP.

This may be used to efficiently clone a subset of an IOAS to another, or as a
kind of ‘cache’ to speed up mapping. Copy has an efficiency advantage over
establishing equivalent new mappings, as internal resources are shared, and
the kernel will pin the user memory only once.

struct iommu\_ioas\_unmap
:   ioctl(IOMMU\_IOAS\_UNMAP)

**Definition**:

```
struct iommu_ioas_unmap {
    __u32 size;
    __u32 ioas_id;
    __aligned_u64 iova;
    __aligned_u64 length;
};
```

**Members**

`size`
:   sizeof([`struct iommu_ioas_unmap`](#c.iommu_ioas_unmap "iommu_ioas_unmap"))

`ioas_id`
:   IOAS ID to change the mapping of

`iova`
:   IOVA to start the unmapping at

`length`
:   Number of bytes to unmap, and return back the bytes unmapped

**Description**

Unmap an IOVA range. The iova/length must be a superset of a previously
mapped range used with IOMMU\_IOAS\_MAP or IOMMU\_IOAS\_COPY. Splitting or
truncating ranges is not allowed. The values 0 to U64\_MAX will unmap
everything.

enum iommufd\_option
:   ioctl(IOMMU\_OPTION\_RLIMIT\_MODE) and ioctl(IOMMU\_OPTION\_HUGE\_PAGES)

**Constants**

`IOMMU_OPTION_RLIMIT_MODE`
:   Change how RLIMIT\_MEMLOCK accounting works. The caller must have privilege
    to invoke this. Value 0 (default) is user based accounting, 1 uses process
    based accounting. Global option, object\_id must be 0

`IOMMU_OPTION_HUGE_PAGES`
:   Value 1 (default) allows contiguous pages to be combined when generating
    iommu mappings. Value 0 disables combining, everything is mapped to
    PAGE\_SIZE. This can be useful for benchmarking. This is a per-IOAS
    option, the object\_id must be the IOAS ID.

enum iommufd\_option\_ops
:   ioctl(IOMMU\_OPTION\_OP\_SET) and ioctl(IOMMU\_OPTION\_OP\_GET)

**Constants**

`IOMMU_OPTION_OP_SET`
:   Set the option’s value

`IOMMU_OPTION_OP_GET`
:   Get the option’s value

struct iommu\_option
:   iommu option multiplexer

**Definition**:

```
struct iommu_option {
    __u32 size;
    __u32 option_id;
    __u16 op;
    __u16 __reserved;
    __u32 object_id;
    __aligned_u64 val64;
};
```

**Members**

`size`
:   sizeof([`struct iommu_option`](#c.iommu_option "iommu_option"))

`option_id`
:   One of [`enum iommufd_option`](#c.iommufd_option "iommufd_option")

`op`
:   One of [`enum iommufd_option_ops`](#c.iommufd_option_ops "iommufd_option_ops")

`__reserved`
:   Must be 0

`object_id`
:   ID of the object if required

`val64`
:   Option value to set or value returned on get

**Description**

Change a simple option value. This multiplexor allows controlling options
on objects. IOMMU\_OPTION\_OP\_SET will load an option and IOMMU\_OPTION\_OP\_GET
will return the current value.

enum iommufd\_vfio\_ioas\_op
:   IOMMU\_VFIO\_IOAS\_\* ioctls

**Constants**

`IOMMU_VFIO_IOAS_GET`
:   Get the current compatibility IOAS

`IOMMU_VFIO_IOAS_SET`
:   Change the current compatibility IOAS

`IOMMU_VFIO_IOAS_CLEAR`
:   Disable VFIO compatibility

struct iommu\_vfio\_ioas
:   ioctl(IOMMU\_VFIO\_IOAS)

**Definition**:

```
struct iommu_vfio_ioas {
    __u32 size;
    __u32 ioas_id;
    __u16 op;
    __u16 __reserved;
};
```

**Members**

`size`
:   sizeof([`struct iommu_vfio_ioas`](#c.iommu_vfio_ioas "iommu_vfio_ioas"))

`ioas_id`
:   For IOMMU\_VFIO\_IOAS\_SET the input IOAS ID to set
    For IOMMU\_VFIO\_IOAS\_GET will output the IOAS ID

`op`
:   One of [`enum iommufd_vfio_ioas_op`](#c.iommufd_vfio_ioas_op "iommufd_vfio_ioas_op")

`__reserved`
:   Must be 0

**Description**

The VFIO compatibility support uses a single ioas because VFIO APIs do not
support the ID field. Set or Get the IOAS that VFIO compatibility will use.
When VFIO\_GROUP\_SET\_CONTAINER is used on an iommufd it will get the
compatibility ioas, either by taking what is already set, or auto creating
one. From then on VFIO will continue to use that ioas and is not effected by
this ioctl. SET or CLEAR does not destroy any auto-created IOAS.

enum iommufd\_hwpt\_alloc\_flags
:   Flags for HWPT allocation

**Constants**

`IOMMU_HWPT_ALLOC_NEST_PARENT`
:   If set, allocate a HWPT that can serve as
    the parent HWPT in a nesting configuration.

`IOMMU_HWPT_ALLOC_DIRTY_TRACKING`
:   Dirty tracking support for device IOMMU is
    enforced on device attachment

`IOMMU_HWPT_FAULT_ID_VALID`
:   The fault\_id field of hwpt allocation data is
    valid.

`IOMMU_HWPT_ALLOC_PASID`
:   Requests a domain that can be used with PASID. The
    domain can be attached to any PASID on the device.
    Any domain attached to the non-PASID part of the
    device must also be flagged, otherwise attaching a
    PASID will blocked.
    For the user that wants to attach PASID, ioas is
    not recommended for both the non-PASID part
    and PASID part of the device.
    If IOMMU does not support PASID it will return
    error (-EOPNOTSUPP).

enum iommu\_hwpt\_vtd\_s1\_flags
:   Intel VT-d stage-1 page table entry attributes

**Constants**

`IOMMU_VTD_S1_SRE`
:   Supervisor request

`IOMMU_VTD_S1_EAFE`
:   Extended access enable

`IOMMU_VTD_S1_WPE`
:   Write protect enable

struct iommu\_hwpt\_vtd\_s1
:   Intel VT-d stage-1 page table info (IOMMU\_HWPT\_DATA\_VTD\_S1)

**Definition**:

```
struct iommu_hwpt_vtd_s1 {
    __aligned_u64 flags;
    __aligned_u64 pgtbl_addr;
    __u32 addr_width;
    __u32 __reserved;
};
```

**Members**

`flags`
:   Combination of [`enum iommu_hwpt_vtd_s1_flags`](#c.iommu_hwpt_vtd_s1_flags "iommu_hwpt_vtd_s1_flags")

`pgtbl_addr`
:   The base address of the stage-1 page table.

`addr_width`
:   The address width of the stage-1 page table

`__reserved`
:   Must be 0

struct iommu\_hwpt\_arm\_smmuv3
:   ARM SMMUv3 nested STE (IOMMU\_HWPT\_DATA\_ARM\_SMMUV3)

**Definition**:

```
struct iommu_hwpt_arm_smmuv3 {
    __aligned_le64 ste[2];
};
```

**Members**

`ste`
:   The first two double words of the user space Stream Table Entry for
    the translation. Must be little-endian.
    Allowed fields: (Refer to “5.2 Stream Table Entry” in SMMUv3 HW Spec)
    - word-0: V, Cfg, S1Fmt, S1ContextPtr, S1CDMax
    - word-1: EATS, S1DSS, S1CIR, S1COR, S1CSH, S1STALLD

**Description**

-EIO will be returned if **ste** is not legal or contains any non-allowed field.
Cfg can be used to select a S1, Bypass or Abort configuration. A Bypass
nested domain will translate the same as the nesting parent. The S1 will
install a Context Descriptor Table pointing at userspace memory translated
by the nesting parent.

It’s suggested to allocate a vDEVICE object carrying vSID and then re-attach
the nested domain, as soon as the vSID is available in the VMM level:

* when Cfg=translate, a vDEVICE must be allocated prior to attaching to the
  allocated nested domain, as CD/ATS invalidations and vevents need a vSID.
* when Cfg=bypass/abort, a vDEVICE is not enforced during the nested domain
  attachment, to support a GBPA case where VM sets CR0.SMMUEN=0. However, if
  VM sets CR0.SMMUEN=1 while missing a vDEVICE object, kernel would fail to
  report events to the VM. E.g. F\_TRANSLATION when guest STE.Cfg=abort.

struct iommu\_hwpt\_amd\_guest
:   AMD IOMMU guest I/O page table data (IOMMU\_HWPT\_DATA\_AMD\_GUEST)

**Definition**:

```
struct iommu_hwpt_amd_guest {
    __aligned_u64 dte[4];
};
```

**Members**

`dte`
:   Guest Device Table Entry (DTE)

enum iommu\_hwpt\_data\_type
:   IOMMU HWPT Data Type

**Constants**

`IOMMU_HWPT_DATA_NONE`
:   no data

`IOMMU_HWPT_DATA_VTD_S1`
:   Intel VT-d stage-1 page table

`IOMMU_HWPT_DATA_ARM_SMMUV3`
:   ARM SMMUv3 Context Descriptor Table

`IOMMU_HWPT_DATA_AMD_GUEST`
:   AMD IOMMU guest page table

struct iommu\_hwpt\_alloc
:   ioctl(IOMMU\_HWPT\_ALLOC)

**Definition**:

```
struct iommu_hwpt_alloc {
    __u32 size;
    __u32 flags;
    __u32 dev_id;
    __u32 pt_id;
    __u32 out_hwpt_id;
    __u32 __reserved;
    __u32 data_type;
    __u32 data_len;
    __aligned_u64 data_uptr;
    __u32 fault_id;
    __u32 __reserved2;
};
```

**Members**

`size`
:   sizeof([`struct iommu_hwpt_alloc`](#c.iommu_hwpt_alloc "iommu_hwpt_alloc"))

`flags`
:   Combination of [`enum iommufd_hwpt_alloc_flags`](#c.iommufd_hwpt_alloc_flags "iommufd_hwpt_alloc_flags")

`dev_id`
:   The device to allocate this HWPT for

`pt_id`
:   The IOAS or HWPT or vIOMMU to connect this HWPT to

`out_hwpt_id`
:   The ID of the new HWPT

`__reserved`
:   Must be 0

`data_type`
:   One of [`enum iommu_hwpt_data_type`](#c.iommu_hwpt_data_type "iommu_hwpt_data_type")

`data_len`
:   Length of the type specific data

`data_uptr`
:   User pointer to the type specific data

`fault_id`
:   The ID of IOMMUFD\_FAULT object. Valid only if flags field of
    IOMMU\_HWPT\_FAULT\_ID\_VALID is set.

`__reserved2`
:   Padding to 64-bit alignment. Must be 0.

**Description**

Explicitly allocate a hardware page table object. This is the same object
type that is returned by [`iommufd_device_attach()`](#c.iommufd_device_attach "iommufd_device_attach") and represents the
underlying iommu driver’s iommu\_domain kernel object.

A kernel-managed HWPT will be created with the mappings from the given
IOAS via the **pt\_id**. The **data\_type** for this allocation must be set to
IOMMU\_HWPT\_DATA\_NONE. The HWPT can be allocated as a parent HWPT for a
nesting configuration by passing IOMMU\_HWPT\_ALLOC\_NEST\_PARENT via **flags**.

A user-managed nested HWPT will be created from a given vIOMMU (wrapping a
parent HWPT) or a parent HWPT via **pt\_id**, in which the parent HWPT must be
allocated previously via the same ioctl from a given IOAS (**pt\_id**). In this
case, the **data\_type** must be set to a pre-defined type corresponding to an
I/O page table type supported by the underlying IOMMU hardware. The device
via **dev\_id** and the vIOMMU via **pt\_id** must be associated to the same IOMMU
instance.

If the **data\_type** is set to IOMMU\_HWPT\_DATA\_NONE, **data\_len** and
**data\_uptr** should be zero. Otherwise, both **data\_len** and **data\_uptr**
must be given.

enum iommu\_hw\_info\_vtd\_flags
:   Flags for VT-d hw\_info

**Constants**

`IOMMU_HW_INFO_VTD_ERRATA_772415_SPR17`
:   If set, disallow read-only mappings
    on a nested\_parent domain.
    <https://www.intel.com/content/www/us/en/content-details/772415/content-details.html>

struct iommu\_hw\_info\_vtd
:   Intel VT-d hardware information

**Definition**:

```
struct iommu_hw_info_vtd {
    __u32 flags;
    __u32 __reserved;
    __aligned_u64 cap_reg;
    __aligned_u64 ecap_reg;
};
```

**Members**

`flags`
:   Combination of [`enum iommu_hw_info_vtd_flags`](#c.iommu_hw_info_vtd_flags "iommu_hw_info_vtd_flags")

`__reserved`
:   Must be 0

`cap_reg`
:   Value of Intel VT-d capability register defined in VT-d spec
    section 11.4.2 Capability Register.

`ecap_reg`
:   Value of Intel VT-d capability register defined in VT-d spec
    section 11.4.3 Extended Capability Register.

**Description**

User needs to understand the Intel VT-d specification to decode the
register value.

struct iommu\_hw\_info\_arm\_smmuv3
:   ARM SMMUv3 hardware information (IOMMU\_HW\_INFO\_TYPE\_ARM\_SMMUV3)

**Definition**:

```
struct iommu_hw_info_arm_smmuv3 {
    __u32 flags;
    __u32 __reserved;
    __u32 idr[6];
    __u32 iidr;
    __u32 aidr;
};
```

**Members**

`flags`
:   Must be set to 0

`__reserved`
:   Must be 0

`idr`
:   Implemented features for ARM SMMU Non-secure programming interface

`iidr`
:   Information about the implementation and implementer of ARM SMMU,
    and architecture version supported

`aidr`
:   ARM SMMU architecture version

**Description**

For the details of **idr**, **iidr** and **aidr**, please refer to the chapters
from 6.3.1 to 6.3.6 in the SMMUv3 Spec.

This reports the raw HW capability, and not all bits are meaningful to be
read by userspace. Only the following fields should be used:

idr[0]: ST\_LEVEL, TERM\_MODEL, STALL\_MODEL, TTENDIAN , CD2L, ASID16, TTF
idr[1]: SIDSIZE, SSIDSIZE
idr[3]: BBML, RIL
idr[5]: VAX, GRAN64K, GRAN16K, GRAN4K

* S1P should be assumed to be true if a NESTED HWPT can be created
* VFIO/iommufd only support platforms with COHACC, it should be assumed to be
  true.
* ATS is a per-device property. If the VMM describes any devices as ATS
  capable in ACPI/DT it should set the corresponding idr.

This list may expand in future (eg E0PD, AIE, PBHA, D128, DS etc). It is
important that VMMs do not read bits outside the list to allow for
compatibility with future kernels. Several features in the SMMUv3
architecture are not currently supported by the kernel for nesting: HTTU,
BTM, MPAM and others.

struct iommu\_hw\_info\_tegra241\_cmdqv
:   NVIDIA Tegra241 CMDQV Hardware Information (IOMMU\_HW\_INFO\_TYPE\_TEGRA241\_CMDQV)

**Definition**:

```
struct iommu_hw_info_tegra241_cmdqv {
    __u32 flags;
    __u8 version;
    __u8 log2vcmdqs;
    __u8 log2vsids;
    __u8 __reserved;
};
```

**Members**

`flags`
:   Must be 0

`version`
:   Version number for the CMDQ-V HW for PARAM bits[03:00]

`log2vcmdqs`
:   Log2 of the total number of VCMDQs for PARAM bits[07:04]

`log2vsids`
:   Log2 of the total number of SID replacements for PARAM bits[15:12]

`__reserved`
:   Must be 0

**Description**

VMM can use these fields directly in its emulated global PARAM register. Note
that only one Virtual Interface (VINTF) should be exposed to a VM, i.e. PARAM
bits[11:08] should be set to 0 for log2 of the total number of VINTFs.

struct iommu\_hw\_info\_amd
:   AMD IOMMU device info

**Definition**:

```
struct iommu_hw_info_amd {
    __aligned_u64 efr;
    __aligned_u64 efr2;
};
```

**Members**

`efr`
:   Value of AMD IOMMU Extended Feature Register (EFR)

`efr2`
:   Value of AMD IOMMU Extended Feature 2 Register (EFR2)

**Description**

Please See description of these registers in the following sections of
the AMD I/O Virtualization Technology (IOMMU) Specification.
(<https://docs.amd.com/v/u/en-US/48882_3.10_PUB>)

* MMIO Offset 0030h IOMMU Extended Feature Register
* MMIO Offset 01A0h IOMMU Extended Feature 2 Register

**Note**

The EFR and EFR2 are raw values reported by hardware.
VMM is responsible to determine the appropriate flags to be exposed to
the VM since cetertain features are not currently supported by the kernel
for HW-vIOMMU.

Current VMM-allowed list of feature flags are:
- EFR[GTSup, GASup, GioSup, PPRSup, EPHSup, GATS, GLX, PASmax]

enum iommu\_hw\_info\_type
:   IOMMU Hardware Info Types

**Constants**

`IOMMU_HW_INFO_TYPE_NONE`
:   Output by the drivers that do not report hardware
    info

`IOMMU_HW_INFO_TYPE_DEFAULT`
:   Input to request for a default type

`IOMMU_HW_INFO_TYPE_INTEL_VTD`
:   Intel VT-d iommu info type

`IOMMU_HW_INFO_TYPE_ARM_SMMUV3`
:   ARM SMMUv3 iommu info type

`IOMMU_HW_INFO_TYPE_TEGRA241_CMDQV`
:   NVIDIA Tegra241 CMDQV (extension for ARM
    SMMUv3) info type

`IOMMU_HW_INFO_TYPE_AMD`
:   AMD IOMMU info type

enum iommufd\_hw\_capabilities

**Constants**

`IOMMU_HW_CAP_DIRTY_TRACKING`
:   IOMMU hardware support for dirty tracking
    If available, it means the following APIs
    are supported:

`IOMMU_HW_CAP_PCI_PASID_EXEC`
:   Execute Permission Supported, user ignores it
    when the [`struct
    iommu_hw_info`](#c.iommu_hw_info "iommu_hw_info")::out\_max\_pasid\_log2 is zero.

`IOMMU_HW_CAP_PCI_PASID_PRIV`
:   Privileged Mode Supported, user ignores it
    when the [`struct
    iommu_hw_info`](#c.iommu_hw_info "iommu_hw_info")::out\_max\_pasid\_log2 is zero.

`IOMMU_HW_CAP_PCI_ATS_NOT_SUPPORTED`
:   ATS is not supported or cannot be used
    on this device (absence implies ATS
    may be enabled)

**Description**

> IOMMU\_HWPT\_GET\_DIRTY\_BITMAP
> IOMMU\_HWPT\_SET\_DIRTY\_TRACKING

enum iommufd\_hw\_info\_flags
:   Flags for iommu\_hw\_info

**Constants**

`IOMMU_HW_INFO_FLAG_INPUT_TYPE`
:   If set, **in\_data\_type** carries an input type
    for user space to request for a specific info

struct iommu\_hw\_info
:   ioctl(IOMMU\_GET\_HW\_INFO)

**Definition**:

```
struct iommu_hw_info {
    __u32 size;
    __u32 flags;
    __u32 dev_id;
    __u32 data_len;
    __aligned_u64 data_uptr;
    union {
        __u32 in_data_type;
        __u32 out_data_type;
    };
    __u8 out_max_pasid_log2;
    __u8 __reserved[3];
    __aligned_u64 out_capabilities;
};
```

**Members**

`size`
:   sizeof([`struct iommu_hw_info`](#c.iommu_hw_info "iommu_hw_info"))

`flags`
:   Must be 0

`dev_id`
:   The device bound to the iommufd

`data_len`
:   Input the length of a user buffer in bytes. Output the length of
    data that kernel supports

`data_uptr`
:   User pointer to a user-space buffer used by the kernel to fill
    the iommu type specific hardware information data

`{unnamed_union}`
:   anonymous

`in_data_type`
:   This shares the same field with **out\_data\_type**, making it be
    a bidirectional field. When IOMMU\_HW\_INFO\_FLAG\_INPUT\_TYPE is
    set, an input type carried via this **in\_data\_type** field will
    be valid, requesting for the info data to the given type. If
    IOMMU\_HW\_INFO\_FLAG\_INPUT\_TYPE is unset, any input value will
    be seen as IOMMU\_HW\_INFO\_TYPE\_DEFAULT

`out_data_type`
:   Output the iommu hardware info type as defined in the [`enum
    iommu_hw_info_type`](#c.iommu_hw_info_type "iommu_hw_info_type").

`out_max_pasid_log2`
:   Output the width of PASIDs. 0 means no PASID support.
    PCI devices turn to out\_capabilities to check if the
    specific capabilities is supported or not.

`__reserved`
:   Must be 0

`out_capabilities`
:   Output the generic iommu capability info type as defined
    in the `enum iommu_hw_capabilities`.

**Description**

Query an iommu type specific hardware information data from an iommu behind
a given device that has been bound to iommufd. This hardware info data will
be used to sync capabilities between the virtual iommu and the physical
iommu, e.g. a nested translation setup needs to check the hardware info, so
a guest stage-1 page table can be compatible with the physical iommu.

To capture an iommu type specific hardware information data, **data\_uptr** and
its length **data\_len** must be provided. Trailing bytes will be zeroed if the
user buffer is larger than the data that kernel has. Otherwise, kernel only
fills the buffer using the given length in **data\_len**. If the ioctl succeeds,
**data\_len** will be updated to the length that kernel actually supports,
**out\_data\_type** will be filled to decode the data filled in the buffer
pointed by **data\_uptr**. Input **data\_len** == zero is allowed.

struct iommu\_hwpt\_set\_dirty\_tracking
:   ioctl(IOMMU\_HWPT\_SET\_DIRTY\_TRACKING)

**Definition**:

```
struct iommu_hwpt_set_dirty_tracking {
    __u32 size;
    __u32 flags;
    __u32 hwpt_id;
    __u32 __reserved;
};
```

**Members**

`size`
:   sizeof([`struct iommu_hwpt_set_dirty_tracking`](#c.iommu_hwpt_set_dirty_tracking "iommu_hwpt_set_dirty_tracking"))

`flags`
:   Combination of `enum iommufd_hwpt_set_dirty_tracking_flags`

`hwpt_id`
:   HW pagetable ID that represents the IOMMU domain

`__reserved`
:   Must be 0

**Description**

Toggle dirty tracking on an HW pagetable.

enum iommufd\_hwpt\_get\_dirty\_bitmap\_flags
:   Flags for getting dirty bits

**Constants**

`IOMMU_HWPT_GET_DIRTY_BITMAP_NO_CLEAR`
:   Just read the PTEs without clearing
    any dirty bits metadata. This flag
    can be passed in the expectation
    where the next operation is an unmap
    of the same IOVA range.

struct iommu\_hwpt\_get\_dirty\_bitmap
:   ioctl(IOMMU\_HWPT\_GET\_DIRTY\_BITMAP)

**Definition**:

```
struct iommu_hwpt_get_dirty_bitmap {
    __u32 size;
    __u32 hwpt_id;
    __u32 flags;
    __u32 __reserved;
    __aligned_u64 iova;
    __aligned_u64 length;
    __aligned_u64 page_size;
    __aligned_u64 data;
};
```

**Members**

`size`
:   sizeof([`struct iommu_hwpt_get_dirty_bitmap`](#c.iommu_hwpt_get_dirty_bitmap "iommu_hwpt_get_dirty_bitmap"))

`hwpt_id`
:   HW pagetable ID that represents the IOMMU domain

`flags`
:   Combination of [`enum iommufd_hwpt_get_dirty_bitmap_flags`](#c.iommufd_hwpt_get_dirty_bitmap_flags "iommufd_hwpt_get_dirty_bitmap_flags")

`__reserved`
:   Must be 0

`iova`
:   base IOVA of the bitmap first bit

`length`
:   IOVA range size

`page_size`
:   page size granularity of each bit in the bitmap

`data`
:   bitmap where to set the dirty bits. The bitmap bits each
    represent a page\_size which you deviate from an arbitrary iova.

**Description**

Checking a given IOVA is dirty:

> data[(iova / page\_size) / 64] & (1ULL << ((iova / page\_size) % 64))

Walk the IOMMU pagetables for a given IOVA range to return a bitmap
with the dirty IOVAs. In doing so it will also by default clear any
dirty bit metadata set in the IOPTE.

enum iommu\_hwpt\_invalidate\_data\_type
:   IOMMU HWPT Cache Invalidation Data Type

**Constants**

`IOMMU_HWPT_INVALIDATE_DATA_VTD_S1`
:   Invalidation data for VTD\_S1

`IOMMU_VIOMMU_INVALIDATE_DATA_ARM_SMMUV3`
:   Invalidation data for ARM SMMUv3

enum iommu\_hwpt\_vtd\_s1\_invalidate\_flags
:   Flags for Intel VT-d stage-1 cache invalidation

**Constants**

`IOMMU_VTD_INV_FLAGS_LEAF`
:   Indicates whether the invalidation applies
    to all-levels page structure cache or just
    the leaf PTE cache.

struct iommu\_hwpt\_vtd\_s1\_invalidate
:   Intel VT-d cache invalidation (IOMMU\_HWPT\_INVALIDATE\_DATA\_VTD\_S1)

**Definition**:

```
struct iommu_hwpt_vtd_s1_invalidate {
    __aligned_u64 addr;
    __aligned_u64 npages;
    __u32 flags;
    __u32 __reserved;
};
```

**Members**

`addr`
:   The start address of the range to be invalidated. It needs to
    be 4KB aligned.

`npages`
:   Number of contiguous 4K pages to be invalidated.

`flags`
:   Combination of [`enum iommu_hwpt_vtd_s1_invalidate_flags`](#c.iommu_hwpt_vtd_s1_invalidate_flags "iommu_hwpt_vtd_s1_invalidate_flags")

`__reserved`
:   Must be 0

**Description**

The Intel VT-d specific invalidation data for user-managed stage-1 cache
invalidation in nested translation. Userspace uses this structure to
tell the impacted cache scope after modifying the stage-1 page table.

Invalidating all the caches related to the page table by setting **addr**
to be 0 and **npages** to be U64\_MAX.

The device TLB will be invalidated automatically if ATS is enabled.

struct iommu\_viommu\_arm\_smmuv3\_invalidate
:   ARM SMMUv3 cache invalidation (IOMMU\_VIOMMU\_INVALIDATE\_DATA\_ARM\_SMMUV3)

**Definition**:

```
struct iommu_viommu_arm_smmuv3_invalidate {
    __aligned_le64 cmd[2];
};
```

**Members**

`cmd`
:   128-bit cache invalidation command that runs in SMMU CMDQ.
    Must be little-endian.

**Description**

Supported command list only when passing in a vIOMMU via **hwpt\_id**:
:   CMDQ\_OP\_TLBI\_NSNH\_ALL
    CMDQ\_OP\_TLBI\_NH\_VA
    CMDQ\_OP\_TLBI\_NH\_VAA
    CMDQ\_OP\_TLBI\_NH\_ALL
    CMDQ\_OP\_TLBI\_NH\_ASID
    CMDQ\_OP\_ATC\_INV
    CMDQ\_OP\_CFGI\_CD
    CMDQ\_OP\_CFGI\_CD\_ALL

-EIO will be returned if the command is not supported.

struct iommu\_hwpt\_invalidate
:   ioctl(IOMMU\_HWPT\_INVALIDATE)

**Definition**:

```
struct iommu_hwpt_invalidate {
    __u32 size;
    __u32 hwpt_id;
    __aligned_u64 data_uptr;
    __u32 data_type;
    __u32 entry_len;
    __u32 entry_num;
    __u32 __reserved;
};
```

**Members**

`size`
:   sizeof([`struct iommu_hwpt_invalidate`](#c.iommu_hwpt_invalidate "iommu_hwpt_invalidate"))

`hwpt_id`
:   ID of a nested HWPT or a vIOMMU, for cache invalidation

`data_uptr`
:   User pointer to an array of driver-specific cache invalidation
    data.

`data_type`
:   One of [`enum iommu_hwpt_invalidate_data_type`](#c.iommu_hwpt_invalidate_data_type "iommu_hwpt_invalidate_data_type"), defining the data
    type of all the entries in the invalidation request array. It
    should be a type supported by the hwpt pointed by **hwpt\_id**.

`entry_len`
:   Length (in bytes) of a request entry in the request array

`entry_num`
:   Input the number of cache invalidation requests in the array.
    Output the number of requests successfully handled by kernel.

`__reserved`
:   Must be 0.

**Description**

Invalidate iommu cache for user-managed page table or vIOMMU. Modifications
on a user-managed page table should be followed by this operation, if a HWPT
is passed in via **hwpt\_id**. Other caches, such as device cache or descriptor
cache can be flushed if a vIOMMU is passed in via the **hwpt\_id** field.

Each ioctl can support one or more cache invalidation requests in the array
that has a total size of **entry\_len** \* **entry\_num**.

An empty invalidation request array by setting **entry\_num\*\*==0 is allowed, and
\*\*entry\_len** and **data\_uptr** would be ignored in this case. This can be used to
check if the given **data\_type** is supported or not by kernel.

enum iommu\_hwpt\_pgfault\_flags
:   flags for [`struct iommu_hwpt_pgfault`](#c.iommu_hwpt_pgfault "iommu_hwpt_pgfault")

**Constants**

`IOMMU_PGFAULT_FLAGS_PASID_VALID`
:   The pasid field of the fault data is
    valid.

`IOMMU_PGFAULT_FLAGS_LAST_PAGE`
:   It’s the last fault of a fault group.

enum iommu\_hwpt\_pgfault\_perm
:   perm bits for [`struct iommu_hwpt_pgfault`](#c.iommu_hwpt_pgfault "iommu_hwpt_pgfault")

**Constants**

`IOMMU_PGFAULT_PERM_READ`
:   request for read permission

`IOMMU_PGFAULT_PERM_WRITE`
:   request for write permission

`IOMMU_PGFAULT_PERM_EXEC`
:   (PCIE 10.4.1) request with a PASID that has the
    Execute Requested bit set in PASID TLP Prefix.

`IOMMU_PGFAULT_PERM_PRIV`
:   (PCIE 10.4.1) request with a PASID that has the
    Privileged Mode Requested bit set in PASID TLP
    Prefix.

struct iommu\_hwpt\_pgfault
:   iommu page fault data

**Definition**:

```
struct iommu_hwpt_pgfault {
    __u32 flags;
    __u32 dev_id;
    __u32 pasid;
    __u32 grpid;
    __u32 perm;
    __u32 __reserved;
    __aligned_u64 addr;
    __u32 length;
    __u32 cookie;
};
```

**Members**

`flags`
:   Combination of [`enum iommu_hwpt_pgfault_flags`](#c.iommu_hwpt_pgfault_flags "iommu_hwpt_pgfault_flags")

`dev_id`
:   id of the originated device

`pasid`
:   Process Address Space ID

`grpid`
:   Page Request Group Index

`perm`
:   Combination of [`enum iommu_hwpt_pgfault_perm`](#c.iommu_hwpt_pgfault_perm "iommu_hwpt_pgfault_perm")

`__reserved`
:   Must be 0.

`addr`
:   Fault address

`length`
:   a hint of how much data the requestor is expecting to fetch. For
    example, if the PRI initiator knows it is going to do a 10MB
    transfer, it could fill in 10MB and the OS could pre-fault in
    10MB of IOVA. It’s default to 0 if there’s no such hint.

`cookie`
:   kernel-managed cookie identifying a group of fault messages. The
    cookie number encoded in the last page fault of the group should
    be echoed back in the response message.

enum iommufd\_page\_response\_code
:   Return status of fault handlers

**Constants**

`IOMMUFD_PAGE_RESP_SUCCESS`
:   Fault has been handled and the page tables
    populated, retry the access. This is the
    “Success” defined in PCI 10.4.2.1.

`IOMMUFD_PAGE_RESP_INVALID`
:   Could not handle this fault, don’t retry the
    access. This is the “Invalid Request” in PCI
    10.4.2.1.

struct iommu\_hwpt\_page\_response
:   IOMMU page fault response

**Definition**:

```
struct iommu_hwpt_page_response {
    __u32 cookie;
    __u32 code;
};
```

**Members**

`cookie`
:   The kernel-managed cookie reported in the fault message.

`code`
:   One of response code in [`enum iommufd_page_response_code`](#c.iommufd_page_response_code "iommufd_page_response_code").

struct iommu\_fault\_alloc
:   ioctl(IOMMU\_FAULT\_QUEUE\_ALLOC)

**Definition**:

```
struct iommu_fault_alloc {
    __u32 size;
    __u32 flags;
    __u32 out_fault_id;
    __u32 out_fault_fd;
};
```

**Members**

`size`
:   sizeof([`struct iommu_fault_alloc`](#c.iommu_fault_alloc "iommu_fault_alloc"))

`flags`
:   Must be 0

`out_fault_id`
:   The ID of the new FAULT

`out_fault_fd`
:   The fd of the new FAULT

**Description**

Explicitly allocate a fault handling object.

enum iommu\_viommu\_type
:   Virtual IOMMU Type

**Constants**

`IOMMU_VIOMMU_TYPE_DEFAULT`
:   Reserved for future use

`IOMMU_VIOMMU_TYPE_ARM_SMMUV3`
:   ARM SMMUv3 driver specific type

`IOMMU_VIOMMU_TYPE_TEGRA241_CMDQV`
:   NVIDIA Tegra241 CMDQV (extension for ARM
    SMMUv3) enabled ARM SMMUv3 type

struct iommu\_viommu\_tegra241\_cmdqv
:   NVIDIA Tegra241 CMDQV Virtual Interface (IOMMU\_VIOMMU\_TYPE\_TEGRA241\_CMDQV)

**Definition**:

```
struct iommu_viommu_tegra241_cmdqv {
    __aligned_u64 out_vintf_mmap_offset;
    __aligned_u64 out_vintf_mmap_length;
};
```

**Members**

`out_vintf_mmap_offset`
:   mmap offset argument for VINTF’s page0

`out_vintf_mmap_length`
:   mmap length argument for VINTF’s page0

**Description**

Both **out\_vintf\_mmap\_offset** and **out\_vintf\_mmap\_length** are reported by kernel
for user space to mmap the VINTF page0 from the host physical address space
to the guest physical address space so that a guest kernel can directly R/W
access to the VINTF page0 in order to control its virtual command queues.

struct iommu\_viommu\_alloc
:   ioctl(IOMMU\_VIOMMU\_ALLOC)

**Definition**:

```
struct iommu_viommu_alloc {
    __u32 size;
    __u32 flags;
    __u32 type;
    __u32 dev_id;
    __u32 hwpt_id;
    __u32 out_viommu_id;
    __u32 data_len;
    __u32 __reserved;
    __aligned_u64 data_uptr;
};
```

**Members**

`size`
:   sizeof([`struct iommu_viommu_alloc`](#c.iommu_viommu_alloc "iommu_viommu_alloc"))

`flags`
:   Must be 0

`type`
:   Type of the virtual IOMMU. Must be defined in [`enum iommu_viommu_type`](#c.iommu_viommu_type "iommu_viommu_type")

`dev_id`
:   The device’s physical IOMMU will be used to back the virtual IOMMU

`hwpt_id`
:   ID of a nesting parent HWPT to associate to

`out_viommu_id`
:   Output virtual IOMMU ID for the allocated object

`data_len`
:   Length of the type specific data

`__reserved`
:   Must be 0

`data_uptr`
:   User pointer to a driver-specific virtual IOMMU data

**Description**

Allocate a virtual IOMMU object, representing the underlying physical IOMMU’s
virtualization support that is a security-isolated slice of the real IOMMU HW
that is unique to a specific VM. Operations global to the IOMMU are connected
to the vIOMMU, such as:
- Security namespace for guest owned ID, e.g. guest-controlled cache tags
- Non-device-affiliated event reporting, e.g. invalidation queue errors
- Access to a sharable nesting parent pagetable across physical IOMMUs
- Virtualization of various platforms IDs, e.g. RIDs and others
- Delivery of paravirtualized invalidation
- Direct assigned invalidation queues
- Direct assigned interrupts

struct iommu\_vdevice\_alloc
:   ioctl(IOMMU\_VDEVICE\_ALLOC)

**Definition**:

```
struct iommu_vdevice_alloc {
    __u32 size;
    __u32 viommu_id;
    __u32 dev_id;
    __u32 out_vdevice_id;
    __aligned_u64 virt_id;
};
```

**Members**

`size`
:   sizeof([`struct iommu_vdevice_alloc`](#c.iommu_vdevice_alloc "iommu_vdevice_alloc"))

`viommu_id`
:   vIOMMU ID to associate with the virtual device

`dev_id`
:   The physical device to allocate a virtual instance on the vIOMMU

`out_vdevice_id`
:   Object handle for the vDevice. Pass to IOMMU\_DESTORY

`virt_id`
:   Virtual device ID per vIOMMU, e.g. vSID of ARM SMMUv3, vDeviceID
    of AMD IOMMU, and vRID of Intel VT-d

**Description**

Allocate a virtual device instance (for a physical device) against a vIOMMU.
This instance holds the device’s information (related to its vIOMMU) in a VM.
User should use IOMMU\_DESTROY to destroy the virtual device before
destroying the physical device (by closing vfio\_cdev fd). Otherwise the
virtual device would be forcibly destroyed on physical device destruction,
its vdevice\_id would be permanently leaked (unremovable & unreusable) until
iommu fd closed.

struct iommu\_ioas\_change\_process
:   ioctl(VFIO\_IOAS\_CHANGE\_PROCESS)

**Definition**:

```
struct iommu_ioas_change_process {
    __u32 size;
    __u32 __reserved;
};
```

**Members**

`size`
:   sizeof([`struct iommu_ioas_change_process`](#c.iommu_ioas_change_process "iommu_ioas_change_process"))

`__reserved`
:   Must be 0

**Description**

This transfers pinned memory counts for every memory map in every IOAS
in the context to the current process. This only supports maps created
with IOMMU\_IOAS\_MAP\_FILE, and returns EINVAL if other maps are present.
If the ioctl returns a failure status, then nothing is changed.

This API is useful for transferring operation of a device from one process
to another, such as during userland live update.

enum iommu\_veventq\_flag
:   flag for [`struct iommufd_vevent_header`](#c.iommufd_vevent_header "iommufd_vevent_header")

**Constants**

`IOMMU_VEVENTQ_FLAG_LOST_EVENTS`
:   vEVENTQ has lost vEVENTs

struct iommufd\_vevent\_header
:   Virtual Event Header for a vEVENTQ Status

**Definition**:

```
struct iommufd_vevent_header {
    __u32 flags;
    __u32 sequence;
};
```

**Members**

`flags`
:   Combination of [`enum iommu_veventq_flag`](#c.iommu_veventq_flag "iommu_veventq_flag")

`sequence`
:   The sequence index of a vEVENT in the vEVENTQ, with a range of
    [0, INT\_MAX] where the following index of INT\_MAX is 0

**Description**

Each iommufd\_vevent\_header reports a sequence index of the following vEVENT:

|  |  |  |  |  |  |
| --- | --- | --- | --- | --- | --- |
| header0 {sequence=0} | data0 | header1 {sequence=1} | data1 | ... | dataN |

And this sequence index is expected to be monotonic to the sequence index of
the previous vEVENT. If two adjacent sequence indexes has a delta larger than
1, it means that delta - 1 number of vEVENTs has lost, e.g. two lost vEVENTs:

|  |  |  |  |  |  |
| --- | --- | --- | --- | --- | --- |
| ... | header3 {sequence=3} | data3 | header6 {sequence=6} | data6 | ... |

If a vEVENT lost at the tail of the vEVENTQ and there is no following vEVENT
providing the next sequence index, an IOMMU\_VEVENTQ\_FLAG\_LOST\_EVENTS header
would be added to the tail, and no data would follow this header:

|  |  |  |  |
| --- | --- | --- | --- |
|  | header3 {sequence=3} | data3 | header4 {flags=LOST\_EVENTS, sequence=4} |

enum iommu\_veventq\_type
:   Virtual Event Queue Type

**Constants**

`IOMMU_VEVENTQ_TYPE_DEFAULT`
:   Reserved for future use

`IOMMU_VEVENTQ_TYPE_ARM_SMMUV3`
:   ARM SMMUv3 Virtual Event Queue

`IOMMU_VEVENTQ_TYPE_TEGRA241_CMDQV`
:   NVIDIA Tegra241 CMDQV Extension IRQ

struct iommu\_vevent\_arm\_smmuv3
:   ARM SMMUv3 Virtual Event (IOMMU\_VEVENTQ\_TYPE\_ARM\_SMMUV3)

**Definition**:

```
struct iommu_vevent_arm_smmuv3 {
    __aligned_le64 evt[4];
};
```

**Members**

`evt`
:   256-bit ARM SMMUv3 Event record, little-endian.
    Reported event records: (Refer to “7.3 Event records” in SMMUv3 HW Spec)
    - 0x04 C\_BAD\_STE
    - 0x06 F\_STREAM\_DISABLED
    - 0x08 C\_BAD\_SUBSTREAMID
    - 0x0a C\_BAD\_CD
    - 0x10 F\_TRANSLATION
    - 0x11 F\_ADDR\_SIZE
    - 0x12 F\_ACCESS
    - 0x13 F\_PERMISSION

**Description**

StreamID field reports a virtual device ID. To receive a virtual event for a
device, a vDEVICE must be allocated via IOMMU\_VDEVICE\_ALLOC.

struct iommu\_vevent\_tegra241\_cmdqv
:   Tegra241 CMDQV IRQ (IOMMU\_VEVENTQ\_TYPE\_TEGRA241\_CMDQV)

**Definition**:

```
struct iommu_vevent_tegra241_cmdqv {
    __aligned_le64 lvcmdq_err_map[2];
};
```

**Members**

`lvcmdq_err_map`
:   128-bit logical vcmdq error map, little-endian.
    (Refer to register LVCMDQ\_ERR\_MAPs per VINTF )

**Description**

The 128-bit register value from HW exclusively reflect the error bits for a
Virtual Interface represented by a vIOMMU object. Read and report directly.

struct iommu\_veventq\_alloc
:   ioctl(IOMMU\_VEVENTQ\_ALLOC)

**Definition**:

```
struct iommu_veventq_alloc {
    __u32 size;
    __u32 flags;
    __u32 viommu_id;
    __u32 type;
    __u32 veventq_depth;
    __u32 out_veventq_id;
    __u32 out_veventq_fd;
    __u32 __reserved;
};
```

**Members**

`size`
:   sizeof([`struct iommu_veventq_alloc`](#c.iommu_veventq_alloc "iommu_veventq_alloc"))

`flags`
:   Must be 0

`viommu_id`
:   virtual IOMMU ID to associate the vEVENTQ with

`type`
:   Type of the vEVENTQ. Must be defined in [`enum iommu_veventq_type`](#c.iommu_veventq_type "iommu_veventq_type")

`veventq_depth`
:   Maximum number of events in the vEVENTQ

`out_veventq_id`
:   The ID of the new vEVENTQ

`out_veventq_fd`
:   The fd of the new vEVENTQ. User space must close the
    successfully returned fd after using it

`__reserved`
:   Must be 0

**Description**

Explicitly allocate a virtual event queue interface for a vIOMMU. A vIOMMU
can have multiple FDs for different types, but is confined to one per **type**.
User space should open the **out\_veventq\_fd** to read vEVENTs out of a vEVENTQ,
if there are vEVENTs available. A vEVENTQ will lose events due to overflow,
if the number of the vEVENTs hits **veventq\_depth**.

Each vEVENT in a vEVENTQ encloses a [`struct iommufd_vevent_header`](#c.iommufd_vevent_header "iommufd_vevent_header") followed by
a type-specific data structure, in a normal case:

|  |  |  |  |  |  |  |  |  |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
|  | header0 | data0 | header1 | data1 | ... | headerN | dataN |  |

unless a tailing IOMMU\_VEVENTQ\_FLAG\_LOST\_EVENTS header is logged (refer to
[`struct iommufd_vevent_header`](#c.iommufd_vevent_header "iommufd_vevent_header")).

enum iommu\_hw\_queue\_type
:   HW Queue Type

**Constants**

`IOMMU_HW_QUEUE_TYPE_DEFAULT`
:   Reserved for future use

`IOMMU_HW_QUEUE_TYPE_TEGRA241_CMDQV`
:   NVIDIA Tegra241 CMDQV (extension for ARM
    SMMUv3) Virtual Command Queue (VCMDQ)

struct iommu\_hw\_queue\_alloc
:   ioctl(IOMMU\_HW\_QUEUE\_ALLOC)

**Definition**:

```
struct iommu_hw_queue_alloc {
    __u32 size;
    __u32 flags;
    __u32 viommu_id;
    __u32 type;
    __u32 index;
    __u32 out_hw_queue_id;
    __aligned_u64 nesting_parent_iova;
    __aligned_u64 length;
};
```

**Members**

`size`
:   sizeof([`struct iommu_hw_queue_alloc`](#c.iommu_hw_queue_alloc "iommu_hw_queue_alloc"))

`flags`
:   Must be 0

`viommu_id`
:   Virtual IOMMU ID to associate the HW queue with

`type`
:   One of [`enum iommu_hw_queue_type`](#c.iommu_hw_queue_type "iommu_hw_queue_type")

`index`
:   The logical index to the HW queue per virtual IOMMU for a multi-queue
    model

`out_hw_queue_id`
:   The ID of the new HW queue

`nesting_parent_iova`
:   Base address of the queue memory in the guest physical
    address space

`length`
:   Length of the queue memory

**Description**

Allocate a HW queue object for a vIOMMU-specific HW-accelerated queue, which
allows HW to access a guest queue memory described using **nesting\_parent\_iova**
and **length**.

A vIOMMU can allocate multiple queues, but it must use a different **index** per
type to separate each allocation, e.g:

```
Type1 HW queue0, Type1 HW queue1, Type2 HW queue0, ...
```

## IOMMUFD Kernel API

The IOMMUFD kAPI is device-centric with group-related tricks managed behind the
scene. This allows the external drivers calling such kAPI to implement a simple
device-centric uAPI for connecting its device to an iommufd, instead of
explicitly imposing the group semantics in its uAPI as VFIO does.

struct iommufd\_device \*iommufd\_device\_bind(struct iommufd\_ctx \*ictx, struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, u32 \*id)
:   Bind a physical device to an iommu fd

**Parameters**

`struct iommufd_ctx *ictx`
:   iommufd file descriptor

`struct device *dev`
:   Pointer to a physical device struct

`u32 *id`
:   Output ID number to return to userspace for this device

**Description**

A successful bind establishes an ownership over the device and returns
`struct iommufd_device` pointer, otherwise returns error pointer.

A driver using this API must set driver\_managed\_dma and must not touch
the device until this routine succeeds and establishes ownership.

Binding a PCI device places the entire RID under iommufd control.

The caller must undo this with [`iommufd_device_unbind()`](#c.iommufd_device_unbind "iommufd_device_unbind")

bool iommufd\_ctx\_has\_group(struct iommufd\_ctx \*ictx, struct iommu\_group \*group)
:   True if any device within the group is bound to the ictx

**Parameters**

`struct iommufd_ctx *ictx`
:   iommufd file descriptor

`struct iommu_group *group`
:   Pointer to a physical iommu\_group struct

**Description**

True if any device within the group has been bound to this ictx, ex. via
[`iommufd_device_bind()`](#c.iommufd_device_bind "iommufd_device_bind"), therefore implying ictx ownership of the group.

void iommufd\_device\_unbind(struct iommufd\_device \*idev)
:   Undo [`iommufd_device_bind()`](#c.iommufd_device_bind "iommufd_device_bind")

**Parameters**

`struct iommufd_device *idev`
:   Device returned by [`iommufd_device_bind()`](#c.iommufd_device_bind "iommufd_device_bind")

**Description**

Release the device from iommufd control. The DMA ownership will return back
to unowned with DMA controlled by the DMA API. This invalidates the
iommufd\_device pointer, other APIs that consume it must not be called
concurrently.

int iommufd\_device\_attach(struct iommufd\_device \*idev, ioasid\_t pasid, u32 \*pt\_id)
:   Connect a device/pasid to an iommu\_domain

**Parameters**

`struct iommufd_device *idev`
:   device to attach

`ioasid_t pasid`
:   pasid to attach

`u32 *pt_id`
:   Input a IOMMUFD\_OBJ\_IOAS, or IOMMUFD\_OBJ\_HWPT\_PAGING
    Output the IOMMUFD\_OBJ\_HWPT\_PAGING ID

**Description**

This connects the device/pasid to an iommu\_domain, either automatically
or manually selected. Once this completes the device could do DMA with
**pasid**. **pasid** is IOMMU\_NO\_PASID if this attach is for no pasid usage.

The caller should return the resulting pt\_id back to userspace.
This function is undone by calling [`iommufd_device_detach()`](#c.iommufd_device_detach "iommufd_device_detach").

int iommufd\_device\_replace(struct iommufd\_device \*idev, ioasid\_t pasid, u32 \*pt\_id)
:   Change the device/pasid’s iommu\_domain

**Parameters**

`struct iommufd_device *idev`
:   device to change

`ioasid_t pasid`
:   pasid to change

`u32 *pt_id`
:   Input a IOMMUFD\_OBJ\_IOAS, or IOMMUFD\_OBJ\_HWPT\_PAGING
    Output the IOMMUFD\_OBJ\_HWPT\_PAGING ID

**Description**

This is the same as:

```
iommufd_device_detach();
iommufd_device_attach();
```

If it fails then no change is made to the attachment. The iommu driver may
implement this so there is no disruption in translation. This can only be
called if [`iommufd_device_attach()`](#c.iommufd_device_attach "iommufd_device_attach") has already succeeded. **pasid** is
IOMMU\_NO\_PASID for no pasid usage.

void iommufd\_device\_detach(struct iommufd\_device \*idev, ioasid\_t pasid)
:   Disconnect a device/device to an iommu\_domain

**Parameters**

`struct iommufd_device *idev`
:   device to detach

`ioasid_t pasid`
:   pasid to detach

**Description**

Undo [`iommufd_device_attach()`](#c.iommufd_device_attach "iommufd_device_attach"). This disconnects the idev from the previously
attached pt\_id. The device returns back to a blocked DMA translation.
**pasid** is IOMMU\_NO\_PASID for no pasid usage.

struct iommufd\_access \*iommufd\_access\_create(struct iommufd\_ctx \*ictx, const struct iommufd\_access\_ops \*ops, void \*data, u32 \*id)
:   Create an iommufd\_access

**Parameters**

`struct iommufd_ctx *ictx`
:   iommufd file descriptor

`const struct iommufd_access_ops *ops`
:   Driver’s ops to associate with the access

`void *data`
:   Opaque data to pass into ops functions

`u32 *id`
:   Output ID number to return to userspace for this access

**Description**

An iommufd\_access allows a driver to read/write to the IOAS without using
DMA. The underlying CPU memory can be accessed using the
[`iommufd_access_pin_pages()`](#c.iommufd_access_pin_pages "iommufd_access_pin_pages") or [`iommufd_access_rw()`](#c.iommufd_access_rw "iommufd_access_rw") functions.

The provided ops are required to use [`iommufd_access_pin_pages()`](#c.iommufd_access_pin_pages "iommufd_access_pin_pages").

void iommufd\_access\_destroy(struct iommufd\_access \*access)
:   Destroy an iommufd\_access

**Parameters**

`struct iommufd_access *access`
:   The access to destroy

**Description**

The caller must stop using the access before destroying it.

void iommufd\_access\_unpin\_pages(struct iommufd\_access \*access, unsigned long iova, unsigned long length)
:   Undo iommufd\_access\_pin\_pages

**Parameters**

`struct iommufd_access *access`
:   IOAS access to act on

`unsigned long iova`
:   Starting IOVA

`unsigned long length`
:   Number of bytes to access

**Description**

Return the `struct page`’s. The caller must stop accessing them before calling
this. The iova/length must exactly match the one provided to access\_pages.

int iommufd\_access\_pin\_pages(struct iommufd\_access \*access, unsigned long iova, unsigned long length, struct page \*\*out\_pages, unsigned int flags)
:   Return a list of pages under the iova

**Parameters**

`struct iommufd_access *access`
:   IOAS access to act on

`unsigned long iova`
:   Starting IOVA

`unsigned long length`
:   Number of bytes to access

`struct page **out_pages`
:   Output page list

`unsigned int flags`
:   IOPMMUFD\_ACCESS\_RW\_\* flags

**Description**

Reads **length** bytes starting at iova and returns the `struct page` \* pointers.
These can be kmap’d by the caller for CPU access.

The caller must perform [`iommufd_access_unpin_pages()`](#c.iommufd_access_unpin_pages "iommufd_access_unpin_pages") when done to balance
this.

This API always requires a page aligned iova. This happens naturally if the
ioas alignment is >= PAGE\_SIZE and the iova is PAGE\_SIZE aligned. However
smaller alignments have corner cases where this API can fail on otherwise
aligned iova.

int iommufd\_access\_rw(struct iommufd\_access \*access, unsigned long iova, void \*data, size\_t length, unsigned int flags)
:   Read or write data under the iova

**Parameters**

`struct iommufd_access *access`
:   IOAS access to act on

`unsigned long iova`
:   Starting IOVA

`void *data`
:   Kernel buffer to copy to/from

`size_t length`
:   Number of bytes to access

`unsigned int flags`
:   IOMMUFD\_ACCESS\_RW\_\* flags

**Description**

Copy kernel to/from data into the range given by IOVA/length. If flags
indicates IOMMUFD\_ACCESS\_RW\_KTHREAD then a large copy can be optimized
by changing it into copy\_to/`from_user()`.

void iommufd\_ctx\_get(struct iommufd\_ctx \*ictx)
:   Get a context reference

**Parameters**

`struct iommufd_ctx *ictx`
:   Context to get

**Description**

The caller must already hold a valid reference to ictx.

struct iommufd\_ctx \*iommufd\_ctx\_from\_file(struct [file](#c.iommufd_ctx_from_file "file") \*file)
:   Acquires a reference to the iommufd context

**Parameters**

`struct file *file`
:   File to obtain the reference from

**Description**

Returns a pointer to the iommufd\_ctx, otherwise ERR\_PTR. The [`struct file`](../filesystems/api-summary.html#c.file "file")
remains owned by the caller and the caller must still do fput. On success
the caller is responsible to call [`iommufd_ctx_put()`](#c.iommufd_ctx_put "iommufd_ctx_put").

struct iommufd\_ctx \*iommufd\_ctx\_from\_fd(int fd)
:   Acquires a reference to the iommufd context

**Parameters**

`int fd`
:   File descriptor to obtain the reference from

**Description**

Returns a pointer to the iommufd\_ctx, otherwise ERR\_PTR. On success
the caller is responsible to call [`iommufd_ctx_put()`](#c.iommufd_ctx_put "iommufd_ctx_put").

void iommufd\_ctx\_put(struct iommufd\_ctx \*ictx)
:   Put back a reference

**Parameters**

`struct iommufd_ctx *ictx`
:   Context to put back

### VFIO and IOMMUFD

Connecting a VFIO device to iommufd can be done in two ways.

First is a VFIO compatible way by directly implementing the /dev/vfio/vfio
container IOCTLs by mapping them into io\_pagetable operations. Doing so allows
the use of iommufd in legacy VFIO applications by symlinking /dev/vfio/vfio to
/dev/iommufd or extending VFIO to SET\_CONTAINER using an iommufd instead of a
container fd.

The second approach directly extends VFIO to support a new set of device-centric
user API based on aforementioned IOMMUFD kernel API. It requires userspace
change but better matches the IOMMUFD API semantics and easier to support new
iommufd features when comparing it to the first approach.

Currently both approaches are still work-in-progress.

There are still a few gaps to be resolved to catch up with VFIO type1, as
documented in `iommufd_vfio_check_extension()`.

## Future TODOs

Currently IOMMUFD supports only kernel-managed I/O page table, similar to VFIO
type1. New features on the radar include:

> * Binding iommu\_domain’s to PASID/SSID
> * Userspace page tables, for ARM, x86 and S390
> * Kernel bypass’d invalidation of user page tables
> * Re-use of the KVM page table in the IOMMU
> * Dirty page tracking in the IOMMU
> * Runtime Increase/Decrease of IOPTE size
> * PRI support with faults resolved in userspace
