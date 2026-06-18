# VME Device Drivers

> 출처(원문): https://docs.kernel.org/driver-api/vme.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# VME Device Drivers

## Driver registration

As with other subsystems within the Linux kernel, VME device drivers register
with the VME subsystem, typically called from the devices init routine. This is
achieved via a call to [`vme_register_driver()`](#c.vme_register_driver "vme_register_driver").

A pointer to a structure of type [`struct vme_driver`](#c.vme_driver "vme_driver") must
be provided to the registration function. Along with the maximum number of
devices your driver is able to support.

At the minimum, the ‘.name’, ‘.match’ and ‘.probe’ elements of
[`struct vme_driver`](#c.vme_driver "vme_driver") should be correctly set. The ‘.name’
element is a pointer to a string holding the device driver’s name.

The ‘.match’ function allows control over which VME devices should be registered
with the driver. The match function should return 1 if a device should be
probed and 0 otherwise. This example match function (from vme\_user.c) limits
the number of devices probed to one:

```
#define USER_BUS_MAX    1
...
static int vme_user_match(struct vme_dev *vdev)
{
        if (vdev->id.num >= USER_BUS_MAX)
                return 0;
        return 1;
}
```

The ‘.probe’ element should contain a pointer to the probe routine. The
probe routine is passed a [`struct vme_dev`](#c.vme_dev "vme_dev") pointer as an
argument.

Here, the ‘num’ field refers to the sequential device ID for this specific
driver. The bridge number (or bus number) can be accessed using
dev->bridge->num.

A function is also provided to unregister the driver from the VME core called
[`vme_unregister_driver()`](#c.vme_unregister_driver "vme_unregister_driver") and should usually be called from the device
driver’s exit routine.

## Resource management

Once a driver has registered with the VME core the provided match routine will
be called the number of times specified during the registration. If a match
succeeds, a non-zero value should be returned. A zero return value indicates
failure. For all successful matches, the probe routine of the corresponding
driver is called. The probe routine is passed a pointer to the devices
device structure. This pointer should be saved, it will be required for
requesting VME resources.

The driver can request ownership of one or more master windows
([`vme_master_request()`](#c.vme_master_request "vme_master_request")), slave windows ([`vme_slave_request()`](#c.vme_slave_request "vme_slave_request"))
and/or dma channels ([`vme_dma_request()`](#c.vme_dma_request "vme_dma_request")). Rather than allowing the device
driver to request a specific window or DMA channel (which may be used by a
different driver) the API allows a resource to be assigned based on the required
attributes of the driver in question. For slave windows these attributes are
split into the VME address spaces that need to be accessed in ‘aspace’ and VME
bus cycle types required in ‘cycle’. Master windows add a further set of
attributes in ‘width’ specifying the required data transfer widths. These
attributes are defined as bitmasks and as such any combination of the
attributes can be requested for a single window, the core will assign a window
that meets the requirements, returning a pointer of type vme\_resource that
should be used to identify the allocated resource when it is used. For DMA
controllers, the request function requires the potential direction of any
transfers to be provided in the route attributes. This is typically VME-to-MEM
and/or MEM-to-VME, though some hardware can support VME-to-VME and MEM-to-MEM
transfers as well as test pattern generation. If an unallocated window fitting
the requirements can not be found a NULL pointer will be returned.

Functions are also provided to free window allocations once they are no longer
required. These functions ([`vme_master_free()`](#c.vme_master_free "vme_master_free"), [`vme_slave_free()`](#c.vme_slave_free "vme_slave_free")
and [`vme_dma_free()`](#c.vme_dma_free "vme_dma_free")) should be passed the pointer to the resource
provided during resource allocation.

## Master windows

Master windows provide access from the local processor[s] out onto the VME bus.
The number of windows available and the available access modes is dependent on
the underlying chipset. A window must be configured before it can be used.

### Master window configuration

Once a master window has been assigned [`vme_master_set()`](#c.vme_master_set "vme_master_set") can be used to
configure it and [`vme_master_get()`](#c.vme_master_get "vme_master_get") to retrieve the current settings. The
address spaces, transfer widths and cycle types are the same as described
under resource management, however some of the options are mutually exclusive.
For example, only one address space may be specified.

### Master window access

The function [`vme_master_read()`](#c.vme_master_read "vme_master_read") can be used to read from and
[`vme_master_write()`](#c.vme_master_write "vme_master_write") used to write to configured master windows.

In addition to simple reads and writes, [`vme_master_rmw()`](#c.vme_master_rmw "vme_master_rmw") is provided to
do a read-modify-write transaction. Parts of a VME window can also be mapped
into user space memory using [`vme_master_mmap_prepare()`](#c.vme_master_mmap_prepare "vme_master_mmap_prepare").

## Slave windows

Slave windows provide devices on the VME bus access into mapped portions of the
local memory. The number of windows available and the access modes that can be
used is dependent on the underlying chipset. A window must be configured before
it can be used.

### Slave window configuration

Once a slave window has been assigned [`vme_slave_set()`](#c.vme_slave_set "vme_slave_set") can be used to
configure it and [`vme_slave_get()`](#c.vme_slave_get "vme_slave_get") to retrieve the current settings.

The address spaces, transfer widths and cycle types are the same as described
under resource management, however some of the options are mutually exclusive.
For example, only one address space may be specified.

### Slave window buffer allocation

Functions are provided to allow the user to allocate
([`vme_alloc_consistent()`](#c.vme_alloc_consistent "vme_alloc_consistent")) and free ([`vme_free_consistent()`](#c.vme_free_consistent "vme_free_consistent"))
contiguous buffers which will be accessible by the VME bridge. These functions
do not have to be used, other methods can be used to allocate a buffer, though
care must be taken to ensure that they are contiguous and accessible by the VME
bridge.

### Slave window access

Slave windows map local memory onto the VME bus, the standard methods for
accessing memory should be used.

## DMA channels

The VME DMA transfer provides the ability to run link-list DMA transfers. The
API introduces the concept of DMA lists. Each DMA list is a link-list which can
be passed to a DMA controller. Multiple lists can be created, extended,
executed, reused and destroyed.

### List Management

The function [`vme_new_dma_list()`](#c.vme_new_dma_list "vme_new_dma_list") is provided to create and
[`vme_dma_list_free()`](#c.vme_dma_list_free "vme_dma_list_free") to destroy DMA lists. Execution of a list will not
automatically destroy the list, thus enabling a list to be reused for repetitive
tasks.

### List Population

An item can be added to a list using [`vme_dma_list_add()`](#c.vme_dma_list_add "vme_dma_list_add") (the source and
destination attributes need to be created before calling this function, this is
covered under “Transfer Attributes”).

Note

The detailed attributes of the transfers source and destination
are not checked until an entry is added to a DMA list, the request
for a DMA channel purely checks the directions in which the
controller is expected to transfer data. As a result it is
possible for this call to return an error, for example if the
source or destination is in an unsupported VME address space.

### Transfer Attributes

The attributes for the source and destination are handled separately from adding
an item to a list. This is due to the diverse attributes required for each type
of source and destination. There are functions to create attributes for PCI, VME
and pattern sources and destinations (where appropriate):

> * PCI source or destination: [`vme_dma_pci_attribute()`](#c.vme_dma_pci_attribute "vme_dma_pci_attribute")
> * VME source or destination: [`vme_dma_vme_attribute()`](#c.vme_dma_vme_attribute "vme_dma_vme_attribute")
> * Pattern source: [`vme_dma_pattern_attribute()`](#c.vme_dma_pattern_attribute "vme_dma_pattern_attribute")

The function [`vme_dma_free_attribute()`](#c.vme_dma_free_attribute "vme_dma_free_attribute") should be used to free an
attribute.

### List Execution

The function [`vme_dma_list_exec()`](#c.vme_dma_list_exec "vme_dma_list_exec") queues a list for execution and will
return once the list has been executed.

## Interrupts

The VME API provides functions to attach and detach callbacks to specific VME
level and status ID combinations and for the generation of VME interrupts with
specific VME level and status IDs.

### Attaching Interrupt Handlers

The function [`vme_irq_request()`](#c.vme_irq_request "vme_irq_request") can be used to attach and
[`vme_irq_free()`](#c.vme_irq_free "vme_irq_free") to free a specific VME level and status ID combination.
Any given combination can only be assigned a single callback function. A void
pointer parameter is provided, the value of which is passed to the callback
function, the use of this pointer is user undefined. The callback parameters are
as follows. Care must be taken in writing a callback function, callback
functions run in interrupt context:

```
void callback(int level, int statid, void *priv);
```

### Interrupt Generation

The function [`vme_irq_generate()`](#c.vme_irq_generate "vme_irq_generate") can be used to generate a VME interrupt
at a given VME level and VME status ID.

## Location monitors

The VME API provides the following functionality to configure the location
monitor.

### Location Monitor Management

The function [`vme_lm_request()`](#c.vme_lm_request "vme_lm_request") is provided to request the use of a block
of location monitors and [`vme_lm_free()`](#c.vme_lm_free "vme_lm_free") to free them after they are no
longer required. Each block may provide a number of location monitors,
monitoring adjacent locations. The function [`vme_lm_count()`](#c.vme_lm_count "vme_lm_count") can be used
to determine how many locations are provided.

### Location Monitor Configuration

Once a bank of location monitors has been allocated, the function
[`vme_lm_set()`](#c.vme_lm_set "vme_lm_set") is provided to configure the location and mode of the
location monitor. The function [`vme_lm_get()`](#c.vme_lm_get "vme_lm_get") can be used to retrieve
existing settings.

### Location Monitor Use

The function [`vme_lm_attach()`](#c.vme_lm_attach "vme_lm_attach") enables a callback to be attached and
[`vme_lm_detach()`](#c.vme_lm_detach "vme_lm_detach") allows on to be detached from each location monitor
location. Each location monitor can monitor a number of adjacent locations. The
callback function is declared as follows.

```
void callback(void *data);
```

## Slot Detection

The function [`vme_slot_num()`](#c.vme_slot_num "vme_slot_num") returns the slot ID of the provided bridge.

## Bus Detection

The function [`vme_bus_num()`](#c.vme_bus_num "vme_bus_num") returns the bus ID of the provided bridge.

## VME API

struct vme\_dev
:   Structure representing a VME device

**Definition**:

```
struct vme_dev {
    int num;
    struct vme_bridge *bridge;
    struct device dev;
    struct list_head drv_list;
    struct list_head bridge_list;
};
```

**Members**

`num`
:   The device number

`bridge`
:   Pointer to the bridge device this device is on

`dev`
:   Internal device structure

`drv_list`
:   List of devices (per driver)

`bridge_list`
:   List of devices (per bridge)

struct vme\_driver
:   Structure representing a VME driver

**Definition**:

```
struct vme_driver {
    const char *name;
    int (*match)(struct vme_dev *vdev);
    int (*probe)(struct vme_dev *vdev);
    void (*remove)(struct vme_dev *vdev);
    struct device_driver driver;
    struct list_head devices;
};
```

**Members**

`name`
:   Driver name, should be unique among VME drivers and usually the same
    as the module name.

`match`
:   Callback used to determine whether probe should be run.

`probe`
:   Callback for device binding, called when new device is detected.

`remove`
:   Callback, called on device removal.

`driver`
:   Underlying generic device driver structure.

`devices`
:   List of VME devices ([`struct vme_dev`](#c.vme_dev "vme_dev")) associated with this driver.

void \*vme\_alloc\_consistent(struct vme\_resource \*resource, size\_t size, dma\_addr\_t \*dma)
:   Allocate contiguous memory.

**Parameters**

`struct vme_resource *resource`
:   Pointer to VME resource.

`size_t size`
:   Size of allocation required.

`dma_addr_t *dma`
:   Pointer to variable to store physical address of allocation.

**Description**

Allocate a contiguous block of memory for use by the driver. This is used to
create the buffers for the slave windows.

**Return**

Virtual address of allocation on success, NULL on failure.

void vme\_free\_consistent(struct vme\_resource \*resource, size\_t size, void \*vaddr, dma\_addr\_t dma)
:   Free previously allocated memory.

**Parameters**

`struct vme_resource *resource`
:   Pointer to VME resource.

`size_t size`
:   Size of allocation to free.

`void *vaddr`
:   Virtual address of allocation.

`dma_addr_t dma`
:   Physical address of allocation.

**Description**

Free previously allocated block of contiguous memory.

size\_t vme\_get\_size(struct vme\_resource \*resource)
:   Helper function returning size of a VME window

**Parameters**

`struct vme_resource *resource`
:   Pointer to VME slave or master resource.

**Description**

Determine the size of the VME window provided. This is a helper
function, wrappering the call to vme\_master\_get or vme\_slave\_get
depending on the type of window resource handed to it.

**Return**

Size of the window on success, zero on failure.

struct vme\_resource \*vme\_slave\_request(struct [vme\_dev](#c.vme_dev "vme_dev") \*vdev, u32 address, u32 cycle)
:   Request a VME slave window resource.

**Parameters**

`struct vme_dev *vdev`
:   Pointer to VME device [`struct vme_dev`](#c.vme_dev "vme_dev") assigned to driver instance.

`u32 address`
:   Required VME address space.

`u32 cycle`
:   Required VME data transfer cycle type.

**Description**

Request use of a VME window resource capable of being set for the requested
address space and data transfer cycle.

**Return**

Pointer to VME resource on success, NULL on failure.

int vme\_slave\_set(struct vme\_resource \*resource, int enabled, unsigned long long vme\_base, unsigned long long size, dma\_addr\_t buf\_base, u32 aspace, u32 cycle)
:   Set VME slave window configuration.

**Parameters**

`struct vme_resource *resource`
:   Pointer to VME slave resource.

`int enabled`
:   State to which the window should be configured.

`unsigned long long vme_base`
:   Base address for the window.

`unsigned long long size`
:   Size of the VME window.

`dma_addr_t buf_base`
:   Based address of buffer used to provide VME slave window storage.

`u32 aspace`
:   VME address space for the VME window.

`u32 cycle`
:   VME data transfer cycle type for the VME window.

**Description**

Set configuration for provided VME slave window.

**Return**

Zero on success, -EINVAL if operation is not supported on this
device, if an invalid resource has been provided or invalid
attributes are provided. Hardware specific errors may also be
returned.

int vme\_slave\_get(struct vme\_resource \*resource, int \*enabled, unsigned long long \*vme\_base, unsigned long long \*size, dma\_addr\_t \*buf\_base, u32 \*aspace, u32 \*cycle)
:   Retrieve VME slave window configuration.

**Parameters**

`struct vme_resource *resource`
:   Pointer to VME slave resource.

`int *enabled`
:   Pointer to variable for storing state.

`unsigned long long *vme_base`
:   Pointer to variable for storing window base address.

`unsigned long long *size`
:   Pointer to variable for storing window size.

`dma_addr_t *buf_base`
:   Pointer to variable for storing slave buffer base address.

`u32 *aspace`
:   Pointer to variable for storing VME address space.

`u32 *cycle`
:   Pointer to variable for storing VME data transfer cycle type.

**Description**

Return configuration for provided VME slave window.

**Return**

Zero on success, -EINVAL if operation is not supported on this
device or if an invalid resource has been provided.

void vme\_slave\_free(struct vme\_resource \*resource)
:   Free VME slave window

**Parameters**

`struct vme_resource *resource`
:   Pointer to VME slave resource.

**Description**

Free the provided slave resource so that it may be reallocated.

struct vme\_resource \*vme\_master\_request(struct [vme\_dev](#c.vme_dev "vme_dev") \*vdev, u32 address, u32 cycle, u32 dwidth)
:   Request a VME master window resource.

**Parameters**

`struct vme_dev *vdev`
:   Pointer to VME device [`struct vme_dev`](#c.vme_dev "vme_dev") assigned to driver instance.

`u32 address`
:   Required VME address space.

`u32 cycle`
:   Required VME data transfer cycle type.

`u32 dwidth`
:   Required VME data transfer width.

**Description**

Request use of a VME window resource capable of being set for the requested
address space, data transfer cycle and width.

**Return**

Pointer to VME resource on success, NULL on failure.

int vme\_master\_set(struct vme\_resource \*resource, int enabled, unsigned long long vme\_base, unsigned long long size, u32 aspace, u32 cycle, u32 dwidth)
:   Set VME master window configuration.

**Parameters**

`struct vme_resource *resource`
:   Pointer to VME master resource.

`int enabled`
:   State to which the window should be configured.

`unsigned long long vme_base`
:   Base address for the window.

`unsigned long long size`
:   Size of the VME window.

`u32 aspace`
:   VME address space for the VME window.

`u32 cycle`
:   VME data transfer cycle type for the VME window.

`u32 dwidth`
:   VME data transfer width for the VME window.

**Description**

Set configuration for provided VME master window.

**Return**

Zero on success, -EINVAL if operation is not supported on this
device, if an invalid resource has been provided or invalid
attributes are provided. Hardware specific errors may also be
returned.

int vme\_master\_get(struct vme\_resource \*resource, int \*enabled, unsigned long long \*vme\_base, unsigned long long \*size, u32 \*aspace, u32 \*cycle, u32 \*dwidth)
:   Retrieve VME master window configuration.

**Parameters**

`struct vme_resource *resource`
:   Pointer to VME master resource.

`int *enabled`
:   Pointer to variable for storing state.

`unsigned long long *vme_base`
:   Pointer to variable for storing window base address.

`unsigned long long *size`
:   Pointer to variable for storing window size.

`u32 *aspace`
:   Pointer to variable for storing VME address space.

`u32 *cycle`
:   Pointer to variable for storing VME data transfer cycle type.

`u32 *dwidth`
:   Pointer to variable for storing VME data transfer width.

**Description**

Return configuration for provided VME master window.

**Return**

Zero on success, -EINVAL if operation is not supported on this
device or if an invalid resource has been provided.

ssize\_t vme\_master\_read(struct vme\_resource \*resource, void \*buf, size\_t count, loff\_t offset)
:   Read data from VME space into a buffer.

**Parameters**

`struct vme_resource *resource`
:   Pointer to VME master resource.

`void *buf`
:   Pointer to buffer where data should be transferred.

`size_t count`
:   Number of bytes to transfer.

`loff_t offset`
:   Offset into VME master window at which to start transfer.

**Description**

Perform read of count bytes of data from location on VME bus which maps into
the VME master window at offset to buf.

**Return**

Number of bytes read, -EINVAL if resource is not a VME master
resource or read operation is not supported. -EFAULT returned if
invalid offset is provided. Hardware specific errors may also be
returned.

ssize\_t vme\_master\_write(struct vme\_resource \*resource, void \*buf, size\_t count, loff\_t offset)
:   Write data out to VME space from a buffer.

**Parameters**

`struct vme_resource *resource`
:   Pointer to VME master resource.

`void *buf`
:   Pointer to buffer holding data to transfer.

`size_t count`
:   Number of bytes to transfer.

`loff_t offset`
:   Offset into VME master window at which to start transfer.

**Description**

Perform write of count bytes of data from buf to location on VME bus which
maps into the VME master window at offset.

**Return**

Number of bytes written, -EINVAL if resource is not a VME master
resource or write operation is not supported. -EFAULT returned if
invalid offset is provided. Hardware specific errors may also be
returned.

unsigned int vme\_master\_rmw(struct vme\_resource \*resource, unsigned int mask, unsigned int compare, unsigned int swap, loff\_t offset)
:   Perform read-modify-write cycle.

**Parameters**

`struct vme_resource *resource`
:   Pointer to VME master resource.

`unsigned int mask`
:   Bits to be compared and swapped in operation.

`unsigned int compare`
:   Bits to be compared with data read from offset.

`unsigned int swap`
:   Bits to be swapped in data read from offset.

`loff_t offset`
:   Offset into VME master window at which to perform operation.

**Description**

Perform read-modify-write cycle on provided location:
- Location on VME bus is read.
- Bits selected by mask are compared with compare.
- Where a selected bit matches that in compare and are selected in swap,
the bit is swapped.
- Result written back to location on VME bus.

**Return**

Bytes written on success, -EINVAL if resource is not a VME master
resource or RMW operation is not supported. Hardware specific
errors may also be returned.

int vme\_master\_mmap\_prepare(struct vme\_resource \*resource, struct vm\_area\_desc \*desc)
:   Mmap region of VME master window.

**Parameters**

`struct vme_resource *resource`
:   Pointer to VME master resource.

`struct vm_area_desc *desc`
:   Pointer to descriptor of user mapping.

**Description**

Memory map a region of the VME master window into user space.

**Return**

Zero on success, -EINVAL if resource is not a VME master
resource or -EFAULT if map exceeds window size. Other generic mmap
errors may also be returned.

void vme\_master\_free(struct vme\_resource \*resource)
:   Free VME master window

**Parameters**

`struct vme_resource *resource`
:   Pointer to VME master resource.

**Description**

Free the provided master resource so that it may be reallocated.

struct vme\_resource \*vme\_dma\_request(struct [vme\_dev](#c.vme_dev "vme_dev") \*vdev, u32 route)
:   Request a DMA controller.

**Parameters**

`struct vme_dev *vdev`
:   Pointer to VME device [`struct vme_dev`](#c.vme_dev "vme_dev") assigned to driver instance.

`u32 route`
:   Required src/destination combination.

**Description**

Request a VME DMA controller with capability to perform transfers between
requested source/destination combination.

**Return**

Pointer to VME DMA resource on success, NULL on failure.

struct vme\_dma\_list \*vme\_new\_dma\_list(struct vme\_resource \*resource)
:   Create new VME DMA list.

**Parameters**

`struct vme_resource *resource`
:   Pointer to VME DMA resource.

**Description**

Create a new VME DMA list. It is the responsibility of the user to free
the list once it is no longer required with [`vme_dma_list_free()`](#c.vme_dma_list_free "vme_dma_list_free").

**Return**

Pointer to new VME DMA list, NULL on allocation failure or invalid
VME DMA resource.

struct vme\_dma\_attr \*vme\_dma\_pattern\_attribute(u32 pattern, u32 type)
:   Create “Pattern” type VME DMA list attribute.

**Parameters**

`u32 pattern`
:   Value to use used as pattern

`u32 type`
:   Type of pattern to be written.

**Description**

Create VME DMA list attribute for pattern generation. It is the
responsibility of the user to free used attributes using
[`vme_dma_free_attribute()`](#c.vme_dma_free_attribute "vme_dma_free_attribute").

**Return**

Pointer to VME DMA attribute, NULL on failure.

struct vme\_dma\_attr \*vme\_dma\_pci\_attribute(dma\_addr\_t address)
:   Create “PCI” type VME DMA list attribute.

**Parameters**

`dma_addr_t address`
:   PCI base address for DMA transfer.

**Description**

Create VME DMA list attribute pointing to a location on PCI for DMA
transfers. It is the responsibility of the user to free used attributes
using [`vme_dma_free_attribute()`](#c.vme_dma_free_attribute "vme_dma_free_attribute").

**Return**

Pointer to VME DMA attribute, NULL on failure.

struct vme\_dma\_attr \*vme\_dma\_vme\_attribute(unsigned long long address, u32 aspace, u32 cycle, u32 dwidth)
:   Create “VME” type VME DMA list attribute.

**Parameters**

`unsigned long long address`
:   VME base address for DMA transfer.

`u32 aspace`
:   VME address space to use for DMA transfer.

`u32 cycle`
:   VME bus cycle to use for DMA transfer.

`u32 dwidth`
:   VME data width to use for DMA transfer.

**Description**

Create VME DMA list attribute pointing to a location on the VME bus for DMA
transfers. It is the responsibility of the user to free used attributes
using [`vme_dma_free_attribute()`](#c.vme_dma_free_attribute "vme_dma_free_attribute").

**Return**

Pointer to VME DMA attribute, NULL on failure.

void vme\_dma\_free\_attribute(struct vme\_dma\_attr \*attributes)
:   Free DMA list attribute.

**Parameters**

`struct vme_dma_attr *attributes`
:   Pointer to DMA list attribute.

**Description**

Free VME DMA list attribute. VME DMA list attributes can be safely freed
once [`vme_dma_list_add()`](#c.vme_dma_list_add "vme_dma_list_add") has returned.

int vme\_dma\_list\_add(struct vme\_dma\_list \*list, struct vme\_dma\_attr \*src, struct vme\_dma\_attr \*dest, size\_t count)
:   Add entry to a VME DMA list.

**Parameters**

`struct vme_dma_list *list`
:   Pointer to VME list.

`struct vme_dma_attr *src`
:   Pointer to DMA list attribute to use as source.

`struct vme_dma_attr *dest`
:   Pointer to DMA list attribute to use as destination.

`size_t count`
:   Number of bytes to transfer.

**Description**

Add an entry to the provided VME DMA list. Entry requires pointers to source
and destination DMA attributes and a count.

Please note, the attributes supported as source and destinations for
transfers are hardware dependent.

**Return**

Zero on success, -EINVAL if operation is not supported on this
device or if the link list has already been submitted for execution.
Hardware specific errors also possible.

int vme\_dma\_list\_exec(struct vme\_dma\_list \*list)
:   Queue a VME DMA list for execution.

**Parameters**

`struct vme_dma_list *list`
:   Pointer to VME list.

**Description**

Queue the provided VME DMA list for execution. The call will return once the
list has been executed.

**Return**

Zero on success, -EINVAL if operation is not supported on this
device. Hardware specific errors also possible.

int vme\_dma\_list\_free(struct vme\_dma\_list \*list)
:   Free a VME DMA list.

**Parameters**

`struct vme_dma_list *list`
:   Pointer to VME list.

**Description**

Free the provided DMA list and all its entries.

**Return**

Zero on success, -EINVAL on invalid VME resource, -EBUSY if resource
is still in use. Hardware specific errors also possible.

int vme\_dma\_free(struct vme\_resource \*resource)
:   Free a VME DMA resource.

**Parameters**

`struct vme_resource *resource`
:   Pointer to VME DMA resource.

**Description**

Free the provided DMA resource so that it may be reallocated.

**Return**

Zero on success, -EINVAL on invalid VME resource, -EBUSY if resource
is still active.

int vme\_irq\_request(struct [vme\_dev](#c.vme_dev "vme_dev") \*vdev, int level, int statid, void (\*callback)(int level, int statid, void \*priv\_data), void \*priv\_data)
:   Request a specific VME interrupt.

**Parameters**

`struct vme_dev *vdev`
:   Pointer to VME device [`struct vme_dev`](#c.vme_dev "vme_dev") assigned to driver instance.

`int level`
:   Interrupt priority being requested.

`int statid`
:   Interrupt vector being requested.

`void (*callback)(int level, int statid, void *priv_data)`
:   Pointer to callback function called when VME interrupt/vector
    received.

`void *priv_data`
:   Generic pointer that will be passed to the callback function.

**Description**

Request callback to be attached as a handler for VME interrupts with provided
level and statid.

**Return**

Zero on success, -EINVAL on invalid vme device, level or if the
function is not supported, -EBUSY if the level/statid combination is
already in use. Hardware specific errors also possible.

void vme\_irq\_free(struct [vme\_dev](#c.vme_dev "vme_dev") \*vdev, int level, int statid)
:   Free a VME interrupt.

**Parameters**

`struct vme_dev *vdev`
:   Pointer to VME device [`struct vme_dev`](#c.vme_dev "vme_dev") assigned to driver instance.

`int level`
:   Interrupt priority of interrupt being freed.

`int statid`
:   Interrupt vector of interrupt being freed.

**Description**

Remove previously attached callback from VME interrupt priority/vector.

int vme\_irq\_generate(struct [vme\_dev](#c.vme_dev "vme_dev") \*vdev, int level, int statid)
:   Generate VME interrupt.

**Parameters**

`struct vme_dev *vdev`
:   Pointer to VME device [`struct vme_dev`](#c.vme_dev "vme_dev") assigned to driver instance.

`int level`
:   Interrupt priority at which to assert the interrupt.

`int statid`
:   Interrupt vector to associate with the interrupt.

**Description**

Generate a VME interrupt of the provided level and with the provided
statid.

**Return**

Zero on success, -EINVAL on invalid vme device, level or if the
function is not supported. Hardware specific errors also possible.

struct vme\_resource \*vme\_lm\_request(struct [vme\_dev](#c.vme_dev "vme_dev") \*vdev)
:   Request a VME location monitor

**Parameters**

`struct vme_dev *vdev`
:   Pointer to VME device [`struct vme_dev`](#c.vme_dev "vme_dev") assigned to driver instance.

**Description**

Allocate a location monitor resource to the driver. A location monitor
allows the driver to monitor accesses to a contiguous number of
addresses on the VME bus.

**Return**

Pointer to a VME resource on success or NULL on failure.

int vme\_lm\_count(struct vme\_resource \*resource)
:   Determine number of VME Addresses monitored

**Parameters**

`struct vme_resource *resource`
:   Pointer to VME location monitor resource.

**Description**

The number of contiguous addresses monitored is hardware dependent.
Return the number of contiguous addresses monitored by the
location monitor.

**Return**

Count of addresses monitored or -EINVAL when provided with an
invalid location monitor resource.

int vme\_lm\_set(struct vme\_resource \*resource, unsigned long long lm\_base, u32 aspace, u32 cycle)
:   Configure location monitor

**Parameters**

`struct vme_resource *resource`
:   Pointer to VME location monitor resource.

`unsigned long long lm_base`
:   Base address to monitor.

`u32 aspace`
:   VME address space to monitor.

`u32 cycle`
:   VME bus cycle type to monitor.

**Description**

Set the base address, address space and cycle type of accesses to be
monitored by the location monitor.

**Return**

Zero on success, -EINVAL when provided with an invalid location
monitor resource or function is not supported. Hardware specific
errors may also be returned.

int vme\_lm\_get(struct vme\_resource \*resource, unsigned long long \*lm\_base, u32 \*aspace, u32 \*cycle)
:   Retrieve location monitor settings

**Parameters**

`struct vme_resource *resource`
:   Pointer to VME location monitor resource.

`unsigned long long *lm_base`
:   Pointer used to output the base address monitored.

`u32 *aspace`
:   Pointer used to output the address space monitored.

`u32 *cycle`
:   Pointer used to output the VME bus cycle type monitored.

**Description**

Retrieve the base address, address space and cycle type of accesses to
be monitored by the location monitor.

**Return**

Zero on success, -EINVAL when provided with an invalid location
monitor resource or function is not supported. Hardware specific
errors may also be returned.

int vme\_lm\_attach(struct vme\_resource \*resource, int monitor, void (\*callback)(void\*), void \*data)
:   Provide callback for location monitor address

**Parameters**

`struct vme_resource *resource`
:   Pointer to VME location monitor resource.

`int monitor`
:   Offset to which callback should be attached.

`void (*callback)(void *)`
:   Pointer to callback function called when triggered.

`void *data`
:   Generic pointer that will be passed to the callback function.

**Description**

Attach a callback to the specified offset into the location monitors
monitored addresses. A generic pointer is provided to allow data to be
passed to the callback when called.

**Return**

Zero on success, -EINVAL when provided with an invalid location
monitor resource or function is not supported. Hardware specific
errors may also be returned.

int vme\_lm\_detach(struct vme\_resource \*resource, int monitor)
:   Remove callback for location monitor address

**Parameters**

`struct vme_resource *resource`
:   Pointer to VME location monitor resource.

`int monitor`
:   Offset to which callback should be removed.

**Description**

Remove the callback associated with the specified offset into the
location monitors monitored addresses.

**Return**

Zero on success, -EINVAL when provided with an invalid location
monitor resource or function is not supported. Hardware specific
errors may also be returned.

void vme\_lm\_free(struct vme\_resource \*resource)
:   Free allocated VME location monitor

**Parameters**

`struct vme_resource *resource`
:   Pointer to VME location monitor resource.

**Description**

Free allocation of a VME location monitor.

WARNING: This function currently expects that any callbacks that have
:   been attached to the location monitor have been removed.

**Return**

Zero on success, -EINVAL when provided with an invalid location
monitor resource.

int vme\_slot\_num(struct [vme\_dev](#c.vme_dev "vme_dev") \*vdev)
:   Retrieve slot ID

**Parameters**

`struct vme_dev *vdev`
:   Pointer to VME device [`struct vme_dev`](#c.vme_dev "vme_dev") assigned to driver instance.

**Description**

Retrieve the slot ID associated with the provided VME device.

**Return**

The slot ID on success, -EINVAL if VME bridge cannot be determined
or the function is not supported. Hardware specific errors may also
be returned.

int vme\_bus\_num(struct [vme\_dev](#c.vme_dev "vme_dev") \*vdev)
:   Retrieve bus number

**Parameters**

`struct vme_dev *vdev`
:   Pointer to VME device [`struct vme_dev`](#c.vme_dev "vme_dev") assigned to driver instance.

**Description**

Retrieve the bus enumeration associated with the provided VME device.

**Return**

The bus number on success, -EINVAL if VME bridge cannot be
determined.

int vme\_register\_driver(struct [vme\_driver](#c.vme_driver "vme_driver") \*drv, unsigned int ndevs)
:   Register a VME driver

**Parameters**

`struct vme_driver *drv`
:   Pointer to VME driver structure to register.

`unsigned int ndevs`
:   Maximum number of devices to allow to be enumerated.

**Description**

Register a VME device driver with the VME subsystem.

**Return**

Zero on success, error value on registration failure.

void vme\_unregister\_driver(struct [vme\_driver](#c.vme_driver "vme_driver") \*drv)
:   Unregister a VME driver

**Parameters**

`struct vme_driver *drv`
:   Pointer to VME driver structure to unregister.

**Description**

Unregister a VME device driver from the VME subsystem.
