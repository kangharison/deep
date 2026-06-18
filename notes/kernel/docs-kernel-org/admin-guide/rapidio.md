# RapidIO Subsystem Guide

> 출처(원문): https://docs.kernel.org/admin-guide/rapidio.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# RapidIO Subsystem Guide

Author:
:   Matt Porter

## Introduction

RapidIO is a high speed switched fabric interconnect with features aimed
at the embedded market. RapidIO provides support for memory-mapped I/O
as well as message-based transactions over the switched fabric network.
RapidIO has a standardized discovery mechanism not unlike the PCI bus
standard that allows simple detection of devices in a network.

This documentation is provided for developers intending to support
RapidIO on new architectures, write new drivers, or to understand the
subsystem internals.

## Known Bugs and Limitations

### Bugs

None. ;)

### Limitations

1. Access/management of RapidIO memory regions is not supported
2. Multiple host enumeration is not supported

## RapidIO driver interface

Drivers are provided a set of calls in order to interface with the
subsystem to gather info on devices, request/map memory region
resources, and manage mailboxes/doorbells.

### Functions

int rio\_local\_read\_config\_32(struct [rio\_mport](#c.rio_mport "rio_mport") \*port, u32 offset, u32 \*data)
:   Read 32 bits from local configuration space

**Parameters**

`struct rio_mport *port`
:   Master port

`u32 offset`
:   Offset into local configuration space

`u32 * data`
:   Pointer to read data into

**Description**

Reads 32 bits of data from the specified offset within the local
device’s configuration space.

int rio\_local\_write\_config\_32(struct [rio\_mport](#c.rio_mport "rio_mport") \*port, u32 offset, u32 data)
:   Write 32 bits to local configuration space

**Parameters**

`struct rio_mport *port`
:   Master port

`u32 offset`
:   Offset into local configuration space

`u32 data`
:   Data to be written

**Description**

Writes 32 bits of data to the specified offset within the local
device’s configuration space.

int rio\_local\_read\_config\_16(struct [rio\_mport](#c.rio_mport "rio_mport") \*port, u32 offset, u16 \*data)
:   Read 16 bits from local configuration space

**Parameters**

`struct rio_mport *port`
:   Master port

`u32 offset`
:   Offset into local configuration space

`u16 * data`
:   Pointer to read data into

**Description**

Reads 16 bits of data from the specified offset within the local
device’s configuration space.

int rio\_local\_write\_config\_16(struct [rio\_mport](#c.rio_mport "rio_mport") \*port, u32 offset, u16 data)
:   Write 16 bits to local configuration space

**Parameters**

`struct rio_mport *port`
:   Master port

`u32 offset`
:   Offset into local configuration space

`u16 data`
:   Data to be written

**Description**

Writes 16 bits of data to the specified offset within the local
device’s configuration space.

int rio\_local\_read\_config\_8(struct [rio\_mport](#c.rio_mport "rio_mport") \*port, u32 offset, u8 \*data)
:   Read 8 bits from local configuration space

**Parameters**

`struct rio_mport *port`
:   Master port

`u32 offset`
:   Offset into local configuration space

`u8 * data`
:   Pointer to read data into

**Description**

Reads 8 bits of data from the specified offset within the local
device’s configuration space.

int rio\_local\_write\_config\_8(struct [rio\_mport](#c.rio_mport "rio_mport") \*port, u32 offset, u8 data)
:   Write 8 bits to local configuration space

**Parameters**

`struct rio_mport *port`
:   Master port

`u32 offset`
:   Offset into local configuration space

`u8 data`
:   Data to be written

**Description**

Writes 8 bits of data to the specified offset within the local
device’s configuration space.

int rio\_read\_config\_32(struct [rio\_dev](#c.rio_dev "rio_dev") \*rdev, u32 offset, u32 \*data)
:   Read 32 bits from configuration space

**Parameters**

`struct rio_dev *rdev`
:   RIO device

`u32 offset`
:   Offset into device configuration space

`u32 * data`
:   Pointer to read data into

**Description**

Reads 32 bits of data from the specified offset within the
RIO device’s configuration space.

int rio\_write\_config\_32(struct [rio\_dev](#c.rio_dev "rio_dev") \*rdev, u32 offset, u32 data)
:   Write 32 bits to configuration space

**Parameters**

`struct rio_dev *rdev`
:   RIO device

`u32 offset`
:   Offset into device configuration space

`u32 data`
:   Data to be written

**Description**

Writes 32 bits of data to the specified offset within the
RIO device’s configuration space.

int rio\_read\_config\_16(struct [rio\_dev](#c.rio_dev "rio_dev") \*rdev, u32 offset, u16 \*data)
:   Read 16 bits from configuration space

**Parameters**

`struct rio_dev *rdev`
:   RIO device

`u32 offset`
:   Offset into device configuration space

`u16 * data`
:   Pointer to read data into

**Description**

Reads 16 bits of data from the specified offset within the
RIO device’s configuration space.

int rio\_write\_config\_16(struct [rio\_dev](#c.rio_dev "rio_dev") \*rdev, u32 offset, u16 data)
:   Write 16 bits to configuration space

**Parameters**

`struct rio_dev *rdev`
:   RIO device

`u32 offset`
:   Offset into device configuration space

`u16 data`
:   Data to be written

**Description**

Writes 16 bits of data to the specified offset within the
RIO device’s configuration space.

int rio\_read\_config\_8(struct [rio\_dev](#c.rio_dev "rio_dev") \*rdev, u32 offset, u8 \*data)
:   Read 8 bits from configuration space

**Parameters**

`struct rio_dev *rdev`
:   RIO device

`u32 offset`
:   Offset into device configuration space

`u8 * data`
:   Pointer to read data into

**Description**

Reads 8 bits of data from the specified offset within the
RIO device’s configuration space.

int rio\_write\_config\_8(struct [rio\_dev](#c.rio_dev "rio_dev") \*rdev, u32 offset, u8 data)
:   Write 8 bits to configuration space

**Parameters**

`struct rio_dev *rdev`
:   RIO device

`u32 offset`
:   Offset into device configuration space

`u8 data`
:   Data to be written

**Description**

Writes 8 bits of data to the specified offset within the
RIO device’s configuration space.

int rio\_send\_doorbell(struct [rio\_dev](#c.rio_dev "rio_dev") \*rdev, u16 data)
:   Send a doorbell message to a device

**Parameters**

`struct rio_dev *rdev`
:   RIO device

`u16 data`
:   Doorbell message data

**Description**

Send a doorbell message to a RIO device. The doorbell message
has a 16-bit info field provided by the **data** argument.

void rio\_init\_mbox\_res(struct resource \*res, int start, int end)
:   Initialize a RIO mailbox resource

**Parameters**

`struct resource *res`
:   resource struct

`int start`
:   start of mailbox range

`int end`
:   end of mailbox range

**Description**

This function is used to initialize the fields of a resource
for use as a mailbox resource. It initializes a range of
mailboxes using the start and end arguments.

void rio\_init\_dbell\_res(struct resource \*res, u16 start, u16 end)
:   Initialize a RIO doorbell resource

**Parameters**

`struct resource *res`
:   resource struct

`u16 start`
:   start of doorbell range

`u16 end`
:   end of doorbell range

**Description**

This function is used to initialize the fields of a resource
for use as a doorbell resource. It initializes a range of
doorbell messages using the start and end arguments.

RIO\_DEVICE

`RIO_DEVICE (dev, ven)`

> macro used to describe a specific RIO device

**Parameters**

`dev`
:   the 16 bit RIO device ID

`ven`
:   the 16 bit RIO vendor ID

**Description**

This macro is used to create a [`struct rio_device_id`](../driver-api/basics.html#c.rio_device_id "rio_device_id") that matches a
specific device. The assembly vendor and assembly device fields
will be set to `RIO_ANY_ID`.

int rio\_add\_outb\_message(struct [rio\_mport](#c.rio_mport "rio_mport") \*mport, struct [rio\_dev](#c.rio_dev "rio_dev") \*rdev, int mbox, void \*buffer, size\_t len)
:   Add RIO message to an outbound mailbox queue

**Parameters**

`struct rio_mport *mport`
:   RIO master port containing the outbound queue

`struct rio_dev *rdev`
:   RIO device the message is be sent to

`int mbox`
:   The outbound mailbox queue

`void *buffer`
:   Pointer to the message buffer

`size_t len`
:   Length of the message buffer

**Description**

Adds a RIO message buffer to an outbound mailbox queue for
transmission. Returns 0 on success.

int rio\_add\_inb\_buffer(struct [rio\_mport](#c.rio_mport "rio_mport") \*mport, int mbox, void \*buffer)
:   Add buffer to an inbound mailbox queue

**Parameters**

`struct rio_mport *mport`
:   Master port containing the inbound mailbox

`int mbox`
:   The inbound mailbox number

`void *buffer`
:   Pointer to the message buffer

**Description**

Adds a buffer to an inbound mailbox queue for reception. Returns
0 on success.

void \*rio\_get\_inb\_message(struct [rio\_mport](#c.rio_mport "rio_mport") \*mport, int mbox)
:   Get A RIO message from an inbound mailbox queue

**Parameters**

`struct rio_mport *mport`
:   Master port containing the inbound mailbox

`int mbox`
:   The inbound mailbox number

**Description**

Get a RIO message from an inbound mailbox queue. Returns 0 on success.

const char \*rio\_name(struct [rio\_dev](#c.rio_dev "rio_dev") \*rdev)
:   Get the unique RIO device identifier

**Parameters**

`struct rio_dev *rdev`
:   RIO device

**Description**

Get the unique RIO device identifier. Returns the device
identifier string.

void \*rio\_get\_drvdata(struct [rio\_dev](#c.rio_dev "rio_dev") \*rdev)
:   Get RIO driver specific data

**Parameters**

`struct rio_dev *rdev`
:   RIO device

**Description**

Get RIO driver specific data. Returns a pointer to the
driver specific data.

void rio\_set\_drvdata(struct [rio\_dev](#c.rio_dev "rio_dev") \*rdev, void \*data)
:   Set RIO driver specific data

**Parameters**

`struct rio_dev *rdev`
:   RIO device

`void *data`
:   Pointer to driver specific data

**Description**

Set RIO driver specific data. device `struct driver` data pointer
is set to the **data** argument.

struct [rio\_dev](#c.rio_dev "rio_dev") \*rio\_dev\_get(struct [rio\_dev](#c.rio_dev "rio_dev") \*rdev)
:   Increments the reference count of the RIO device structure

**Parameters**

`struct rio_dev *rdev`
:   RIO device being referenced

**Description**

Each live reference to a device should be refcounted.

Drivers for RIO devices should normally record such references in
their `probe()` methods, when they bind to a device, and release
them by calling [`rio_dev_put()`](#c.rio_dev_put "rio_dev_put"), in their `disconnect()` methods.

void rio\_dev\_put(struct [rio\_dev](#c.rio_dev "rio_dev") \*rdev)
:   Release a use of the RIO device structure

**Parameters**

`struct rio_dev *rdev`
:   RIO device being disconnected

**Description**

Must be called when a user of a device is finished with it.
When the last user of the device calls this function, the
memory of the device is freed.

int rio\_register\_driver(struct [rio\_driver](#c.rio_driver "rio_driver") \*rdrv)
:   register a new RIO driver

**Parameters**

`struct rio_driver *rdrv`
:   the RIO driver structure to register

**Description**

> Adds a [`struct rio_driver`](#c.rio_driver "rio_driver") to the list of registered drivers.
> Returns a negative value on error, otherwise 0. If no error
> occurred, the driver remains registered even if no device
> was claimed during registration.

void rio\_unregister\_driver(struct [rio\_driver](#c.rio_driver "rio_driver") \*rdrv)
:   unregister a RIO driver

**Parameters**

`struct rio_driver *rdrv`
:   the RIO driver structure to unregister

**Description**

> Deletes the [`struct rio_driver`](#c.rio_driver "rio_driver") from the list of registered RIO
> drivers, gives it a chance to clean up by calling its `remove()`
> function for each device it was responsible for, and marks those
> devices as driverless.

u16 rio\_local\_get\_device\_id(struct [rio\_mport](#c.rio_mport "rio_mport") \*port)
:   Get the base/extended device id for a port

**Parameters**

`struct rio_mport *port`
:   RIO master port from which to get the deviceid

**Description**

Reads the base/extended device id from the local device
implementing the master port. Returns the 8/16-bit device
id.

int rio\_query\_mport(struct [rio\_mport](#c.rio_mport "rio_mport") \*port, struct [rio\_mport\_attr](#c.rio_mport_attr "rio_mport_attr") \*mport\_attr)
:   Query mport device attributes

**Parameters**

`struct rio_mport *port`
:   mport device to query

`struct rio_mport_attr *mport_attr`
:   mport attributes data structure

**Description**

Returns attributes of specified mport through the
pointer to attributes data structure.

struct [rio\_net](#c.rio_net "rio_net") \*rio\_alloc\_net(struct [rio\_mport](#c.rio_mport "rio_mport") \*mport)
:   Allocate and initialize a new RIO network data structure

**Parameters**

`struct rio_mport *mport`
:   Master port associated with the RIO network

**Description**

Allocates a RIO network structure, initializes per-network
list heads, and adds the associated master port to the
network list of associated master ports. Returns a
RIO network pointer on success or `NULL` on failure.

void rio\_local\_set\_device\_id(struct [rio\_mport](#c.rio_mport "rio_mport") \*port, u16 did)
:   Set the base/extended device id for a port

**Parameters**

`struct rio_mport *port`
:   RIO master port

`u16 did`
:   Device ID value to be written

**Description**

Writes the base/extended device id from a device.

int rio\_add\_device(struct [rio\_dev](#c.rio_dev "rio_dev") \*rdev)
:   Adds a RIO device to the device model

**Parameters**

`struct rio_dev *rdev`
:   RIO device

**Description**

Adds the RIO device to the global device list and adds the RIO
device to the RIO device list. Creates the generic sysfs nodes
for an RIO device.

int rio\_request\_inb\_mbox(struct [rio\_mport](#c.rio_mport "rio_mport") \*mport, void \*dev\_id, int mbox, int entries, void (\*minb)(struct [rio\_mport](#c.rio_mport "rio_mport") \*mport, void \*dev\_id, int mbox, int slot))
:   request inbound mailbox service

**Parameters**

`struct rio_mport *mport`
:   RIO master port from which to allocate the mailbox resource

`void *dev_id`
:   Device specific pointer to pass on event

`int mbox`
:   Mailbox number to claim

`int entries`
:   Number of entries in inbound mailbox queue

`void (*minb) (struct rio_mport * mport, void *dev_id, int mbox, int slot)`
:   Callback to execute when inbound message is received

**Description**

Requests ownership of an inbound mailbox resource and binds
a callback function to the resource. Returns `0` on success.

int rio\_release\_inb\_mbox(struct [rio\_mport](#c.rio_mport "rio_mport") \*mport, int mbox)
:   release inbound mailbox message service

**Parameters**

`struct rio_mport *mport`
:   RIO master port from which to release the mailbox resource

`int mbox`
:   Mailbox number to release

**Description**

Releases ownership of an inbound mailbox resource. Returns 0
if the request has been satisfied.

int rio\_request\_outb\_mbox(struct [rio\_mport](#c.rio_mport "rio_mport") \*mport, void \*dev\_id, int mbox, int entries, void (\*moutb)(struct [rio\_mport](#c.rio_mport "rio_mport") \*mport, void \*dev\_id, int mbox, int slot))
:   request outbound mailbox service

**Parameters**

`struct rio_mport *mport`
:   RIO master port from which to allocate the mailbox resource

`void *dev_id`
:   Device specific pointer to pass on event

`int mbox`
:   Mailbox number to claim

`int entries`
:   Number of entries in outbound mailbox queue

`void (*moutb) (struct rio_mport * mport, void *dev_id, int mbox, int slot)`
:   Callback to execute when outbound message is sent

**Description**

Requests ownership of an outbound mailbox resource and binds
a callback function to the resource. Returns 0 on success.

int rio\_release\_outb\_mbox(struct [rio\_mport](#c.rio_mport "rio_mport") \*mport, int mbox)
:   release outbound mailbox message service

**Parameters**

`struct rio_mport *mport`
:   RIO master port from which to release the mailbox resource

`int mbox`
:   Mailbox number to release

**Description**

Releases ownership of an inbound mailbox resource. Returns 0
if the request has been satisfied.

int rio\_request\_inb\_dbell(struct [rio\_mport](#c.rio_mport "rio_mport") \*mport, void \*dev\_id, u16 start, u16 end, void (\*dinb)(struct [rio\_mport](#c.rio_mport "rio_mport") \*mport, void \*dev\_id, u16 src, u16 dst, u16 info))
:   request inbound doorbell message service

**Parameters**

`struct rio_mport *mport`
:   RIO master port from which to allocate the doorbell resource

`void *dev_id`
:   Device specific pointer to pass on event

`u16 start`
:   Doorbell info range start

`u16 end`
:   Doorbell info range end

`void (*dinb) (struct rio_mport * mport, void *dev_id, u16 src, u16 dst, u16 info)`
:   Callback to execute when doorbell is received

**Description**

Requests ownership of an inbound doorbell resource and binds
a callback function to the resource. Returns 0 if the request
has been satisfied.

int rio\_release\_inb\_dbell(struct [rio\_mport](#c.rio_mport "rio_mport") \*mport, u16 start, u16 end)
:   release inbound doorbell message service

**Parameters**

`struct rio_mport *mport`
:   RIO master port from which to release the doorbell resource

`u16 start`
:   Doorbell info range start

`u16 end`
:   Doorbell info range end

**Description**

Releases ownership of an inbound doorbell resource and removes
callback from the doorbell event list. Returns 0 if the request
has been satisfied.

struct resource \*rio\_request\_outb\_dbell(struct [rio\_dev](#c.rio_dev "rio_dev") \*rdev, u16 start, u16 end)
:   request outbound doorbell message range

**Parameters**

`struct rio_dev *rdev`
:   RIO device from which to allocate the doorbell resource

`u16 start`
:   Doorbell message range start

`u16 end`
:   Doorbell message range end

**Description**

Requests ownership of a doorbell message range. Returns a resource
if the request has been satisfied or `NULL` on failure.

int rio\_release\_outb\_dbell(struct [rio\_dev](#c.rio_dev "rio_dev") \*rdev, struct resource \*res)
:   release outbound doorbell message range

**Parameters**

`struct rio_dev *rdev`
:   RIO device from which to release the doorbell resource

`struct resource *res`
:   Doorbell resource to be freed

**Description**

Releases ownership of a doorbell message range. Returns 0 if the
request has been satisfied.

int rio\_add\_mport\_pw\_handler(struct [rio\_mport](#c.rio_mport "rio_mport") \*mport, void \*context, int (\*pwcback)(struct [rio\_mport](#c.rio_mport "rio_mport") \*mport, void \*context, union rio\_pw\_msg \*msg, int step))
:   add port-write message handler into the list of mport specific pw handlers

**Parameters**

`struct rio_mport *mport`
:   RIO master port to bind the portwrite callback

`void *context`
:   Handler specific context to pass on event

`int (*pwcback)(struct rio_mport *mport, void *context, union rio_pw_msg *msg, int step)`
:   Callback to execute when portwrite is received

**Description**

Returns 0 if the request has been satisfied.

int rio\_del\_mport\_pw\_handler(struct [rio\_mport](#c.rio_mport "rio_mport") \*mport, void \*context, int (\*pwcback)(struct [rio\_mport](#c.rio_mport "rio_mport") \*mport, void \*context, union rio\_pw\_msg \*msg, int step))
:   remove port-write message handler from the list of mport specific pw handlers

**Parameters**

`struct rio_mport *mport`
:   RIO master port to bind the portwrite callback

`void *context`
:   Registered handler specific context to pass on event

`int (*pwcback)(struct rio_mport *mport, void *context, union rio_pw_msg *msg, int step)`
:   Registered callback function

**Description**

Returns 0 if the request has been satisfied.

int rio\_request\_inb\_pwrite(struct [rio\_dev](#c.rio_dev "rio_dev") \*rdev, int (\*pwcback)(struct [rio\_dev](#c.rio_dev "rio_dev") \*rdev, union rio\_pw\_msg \*msg, int step))
:   request inbound port-write message service for specific RapidIO device

**Parameters**

`struct rio_dev *rdev`
:   RIO device to which register inbound port-write callback routine

`int (*pwcback)(struct rio_dev *rdev, union rio_pw_msg *msg, int step)`
:   Callback routine to execute when port-write is received

**Description**

Binds a port-write callback function to the RapidIO device.
Returns 0 if the request has been satisfied.

int rio\_release\_inb\_pwrite(struct [rio\_dev](#c.rio_dev "rio_dev") \*rdev)
:   release inbound port-write message service associated with specific RapidIO device

**Parameters**

`struct rio_dev *rdev`
:   RIO device which registered for inbound port-write callback

**Description**

Removes callback from the rio\_dev structure. Returns 0 if the request
has been satisfied.

void rio\_pw\_enable(struct [rio\_mport](#c.rio_mport "rio_mport") \*mport, int enable)
:   Enables/disables port-write handling by a master port

**Parameters**

`struct rio_mport *mport`
:   Master port associated with port-write handling

`int enable`
:   1=enable, 0=disable

int rio\_map\_inb\_region(struct [rio\_mport](#c.rio_mport "rio_mport") \*mport, dma\_addr\_t local, u64 rbase, u32 size, u32 rflags)
:   * Map inbound memory region.

**Parameters**

`struct rio_mport *mport`
:   Master port.

`dma_addr_t local`
:   physical address of memory region to be mapped

`u64 rbase`
:   RIO base address assigned to this window

`u32 size`
:   Size of the memory region

`u32 rflags`
:   Flags for mapping.

**Return**

0 -- Success.

**Description**

This function will create the mapping from RIO space to local memory.

void rio\_unmap\_inb\_region(struct [rio\_mport](#c.rio_mport "rio_mport") \*mport, dma\_addr\_t lstart)
:   * Unmap the inbound memory region

**Parameters**

`struct rio_mport *mport`
:   Master port

`dma_addr_t lstart`
:   physical address of memory region to be unmapped

int rio\_map\_outb\_region(struct [rio\_mport](#c.rio_mport "rio_mport") \*mport, u16 destid, u64 rbase, u32 size, u32 rflags, dma\_addr\_t \*local)
:   * Map outbound memory region.

**Parameters**

`struct rio_mport *mport`
:   Master port.

`u16 destid`
:   destination id window points to

`u64 rbase`
:   RIO base address window translates to

`u32 size`
:   Size of the memory region

`u32 rflags`
:   Flags for mapping.

`dma_addr_t *local`
:   physical address of memory region mapped

**Return**

0 -- Success.

**Description**

This function will create the mapping from RIO space to local memory.

void rio\_unmap\_outb\_region(struct [rio\_mport](#c.rio_mport "rio_mport") \*mport, u16 destid, u64 rstart)
:   * Unmap the inbound memory region

**Parameters**

`struct rio_mport *mport`
:   Master port

`u16 destid`
:   destination id mapping points to

`u64 rstart`
:   RIO base address window translates to

u32 rio\_mport\_get\_physefb(struct [rio\_mport](#c.rio_mport "rio_mport") \*port, int local, u16 destid, u8 hopcount, u32 \*rmap)
:   Helper function that returns register offset for Physical Layer Extended Features Block.

**Parameters**

`struct rio_mport *port`
:   Master port to issue transaction

`int local`
:   Indicate a local master port or remote device access

`u16 destid`
:   Destination ID of the device

`u8 hopcount`
:   Number of switch hops to the device

`u32 *rmap`
:   pointer to location to store register map type info

struct [rio\_dev](#c.rio_dev "rio_dev") \*rio\_get\_comptag(u32 comp\_tag, struct [rio\_dev](#c.rio_dev "rio_dev") \*from)
:   Begin or continue searching for a RIO device by component tag

**Parameters**

`u32 comp_tag`
:   RIO component tag to match

`struct rio_dev *from`
:   Previous RIO device found in search, or `NULL` for new search

**Description**

Iterates through the list of known RIO devices. If a RIO device is
found with a matching **comp\_tag**, a pointer to its device
structure is returned. Otherwise, `NULL` is returned. A new search
is initiated by passing `NULL` to the **from** argument. Otherwise, if
**from** is not `NULL`, searches continue from next device on the global
list.

int rio\_set\_port\_lockout(struct [rio\_dev](#c.rio_dev "rio_dev") \*rdev, u32 pnum, int lock)
:   Sets/clears LOCKOUT bit (RIO EM 1.3) for a switch port.

**Parameters**

`struct rio_dev *rdev`
:   Pointer to RIO device control structure

`u32 pnum`
:   Switch port number to set LOCKOUT bit

`int lock`
:   Operation : set (=1) or clear (=0)

int rio\_enable\_rx\_tx\_port(struct [rio\_mport](#c.rio_mport "rio_mport") \*port, int local, u16 destid, u8 hopcount, u8 port\_num)
:   enable input receiver and output transmitter of given port

**Parameters**

`struct rio_mport *port`
:   Master port associated with the RIO network

`int local`
:   local=1 select local port otherwise a far device is reached

`u16 destid`
:   Destination ID of the device to check host bit

`u8 hopcount`
:   Number of hops to reach the target

`u8 port_num`
:   Port (-number on switch) to enable on a far end device

**Description**

Returns 0 or 1 from on General Control Command and Status Register
(EXT\_PTR+0x3C)

int rio\_mport\_chk\_dev\_access(struct [rio\_mport](#c.rio_mport "rio_mport") \*mport, u16 destid, u8 hopcount)
:   Validate access to the specified device.

**Parameters**

`struct rio_mport *mport`
:   Master port to send transactions

`u16 destid`
:   Device destination ID in network

`u8 hopcount`
:   Number of hops into the network

int rio\_inb\_pwrite\_handler(struct [rio\_mport](#c.rio_mport "rio_mport") \*mport, union rio\_pw\_msg \*pw\_msg)
:   inbound port-write message handler

**Parameters**

`struct rio_mport *mport`
:   mport device associated with port-write

`union rio_pw_msg *pw_msg`
:   pointer to inbound port-write message

**Description**

Processes an inbound port-write message. Returns 0 if the request
has been satisfied.

u32 rio\_mport\_get\_efb(struct [rio\_mport](#c.rio_mport "rio_mport") \*port, int local, u16 destid, u8 hopcount, u32 from)
:   get pointer to next extended features block

**Parameters**

`struct rio_mport *port`
:   Master port to issue transaction

`int local`
:   Indicate a local master port or remote device access

`u16 destid`
:   Destination ID of the device

`u8 hopcount`
:   Number of switch hops to the device

`u32 from`
:   Offset of current Extended Feature block header (if 0 starts
    from ExtFeaturePtr)

u32 rio\_mport\_get\_feature(struct [rio\_mport](#c.rio_mport "rio_mport") \*port, int local, u16 destid, u8 hopcount, int ftr)
:   query for devices’ extended features

**Parameters**

`struct rio_mport * port`
:   Master port to issue transaction

`int local`
:   Indicate a local master port or remote device access

`u16 destid`
:   Destination ID of the device

`u8 hopcount`
:   Number of switch hops to the device

`int ftr`
:   Extended feature code

**Description**

Tell if a device supports a given RapidIO capability.
Returns the offset of the requested extended feature
block within the device’s RIO configuration space or
0 in case the device does not support it.

int rio\_lock\_device(struct [rio\_mport](#c.rio_mport "rio_mport") \*port, u16 destid, u8 hopcount, int wait\_ms)
:   Acquires host device lock for specified device

**Parameters**

`struct rio_mport *port`
:   Master port to send transaction

`u16 destid`
:   Destination ID for device/switch

`u8 hopcount`
:   Hopcount to reach switch

`int wait_ms`
:   Max wait time in msec (0 = no timeout)

**Description**

Attepts to acquire host device lock for specified device
Returns 0 if device lock acquired or EINVAL if timeout expires.

int rio\_unlock\_device(struct [rio\_mport](#c.rio_mport "rio_mport") \*port, u16 destid, u8 hopcount)
:   Releases host device lock for specified device

**Parameters**

`struct rio_mport *port`
:   Master port to send transaction

`u16 destid`
:   Destination ID for device/switch

`u8 hopcount`
:   Hopcount to reach switch

**Description**

Returns 0 if device lock released or EINVAL if fails.

int rio\_route\_add\_entry(struct [rio\_dev](#c.rio_dev "rio_dev") \*rdev, u16 table, u16 route\_destid, u8 route\_port, int lock)
:   Add a route entry to a switch routing table

**Parameters**

`struct rio_dev *rdev`
:   RIO device

`u16 table`
:   Routing table ID

`u16 route_destid`
:   Destination ID to be routed

`u8 route_port`
:   Port number to be routed

`int lock`
:   apply a hardware lock on switch device flag (1=lock, 0=no\_lock)

**Description**

If available calls the switch specific `add_entry()` method to add a route
entry into a switch routing table. Otherwise uses standard RT update method
as defined by RapidIO specification. A specific routing table can be selected
using the **table** argument if a switch has per port routing tables or
the standard (or global) table may be used by passing
`RIO_GLOBAL_TABLE` in **table**.

Returns `0` on success or `-EINVAL` on failure.

int rio\_route\_get\_entry(struct [rio\_dev](#c.rio_dev "rio_dev") \*rdev, u16 table, u16 route\_destid, u8 \*route\_port, int lock)
:   Read an entry from a switch routing table

**Parameters**

`struct rio_dev *rdev`
:   RIO device

`u16 table`
:   Routing table ID

`u16 route_destid`
:   Destination ID to be routed

`u8 *route_port`
:   Pointer to read port number into

`int lock`
:   apply a hardware lock on switch device flag (1=lock, 0=no\_lock)

**Description**

If available calls the switch specific `get_entry()` method to fetch a route
entry from a switch routing table. Otherwise uses standard RT read method
as defined by RapidIO specification. A specific routing table can be selected
using the **table** argument if a switch has per port routing tables or
the standard (or global) table may be used by passing
`RIO_GLOBAL_TABLE` in **table**.

Returns `0` on success or `-EINVAL` on failure.

int rio\_route\_clr\_table(struct [rio\_dev](#c.rio_dev "rio_dev") \*rdev, u16 table, int lock)
:   Clear a switch routing table

**Parameters**

`struct rio_dev *rdev`
:   RIO device

`u16 table`
:   Routing table ID

`int lock`
:   apply a hardware lock on switch device flag (1=lock, 0=no\_lock)

**Description**

If available calls the switch specific `clr_table()` method to clear a switch
routing table. Otherwise uses standard RT write method as defined by RapidIO
specification. A specific routing table can be selected using the **table**
argument if a switch has per port routing tables or the standard (or global)
table may be used by passing `RIO_GLOBAL_TABLE` in **table**.

Returns `0` on success or `-EINVAL` on failure.

struct dma\_chan \*rio\_request\_mport\_dma(struct [rio\_mport](#c.rio_mport "rio_mport") \*mport)
:   request RapidIO capable DMA channel associated with specified local RapidIO mport device.

**Parameters**

`struct rio_mport *mport`
:   RIO mport to perform DMA data transfers

**Description**

Returns pointer to allocated DMA channel or NULL if failed.

void rio\_release\_dma(struct dma\_chan \*dchan)
:   release specified DMA channel

**Parameters**

`struct dma_chan *dchan`
:   DMA channel to release

struct dma\_async\_tx\_descriptor \*rio\_dma\_prep\_xfer(struct dma\_chan \*dchan, u16 destid, struct rio\_dma\_data \*data, enum dma\_transfer\_direction direction, unsigned long flags)
:   RapidIO specific wrapper for device\_prep\_slave\_sg callback defined by DMAENGINE.

**Parameters**

`struct dma_chan *dchan`
:   DMA channel to configure

`u16 destid`
:   target RapidIO device destination ID

`struct rio_dma_data *data`
:   RIO specific data descriptor

`enum dma_transfer_direction direction`
:   DMA data transfer direction (TO or FROM the device)

`unsigned long flags`
:   dmaengine defined flags

**Description**

Initializes RapidIO capable DMA channel for the specified data transfer.
Uses DMA channel private extension to pass information related to remote
target RIO device.

**Return**

pointer to DMA transaction descriptor if successful,
error-valued pointer or NULL if failed.

int rio\_register\_scan(int mport\_id, struct [rio\_scan](#c.rio_scan "rio_scan") \*scan\_ops)
:   enumeration/discovery method registration interface

**Parameters**

`int mport_id`
:   mport device ID for which fabric scan routine has to be set
    (RIO\_MPORT\_ANY = set for all available mports)

`struct rio_scan *scan_ops`
:   enumeration/discovery operations structure

**Description**

Registers enumeration/discovery operations with RapidIO subsystem and
attaches it to the specified mport device (or all available mports
if RIO\_MPORT\_ANY is specified).

Returns error if the mport already has an enumerator attached to it.
In case of RIO\_MPORT\_ANY skips mports with valid scan routines (no error).

## Internals

This chapter contains the autogenerated documentation of the RapidIO
subsystem.

### Structures

struct rio\_switch
:   RIO switch info

**Definition**:

```
struct rio_switch {
    struct list_head node;
    u8 *route_table;
    u32 port_ok;
    struct rio_switch_ops *ops;
    spinlock_t lock;
    struct rio_dev *nextdev[];
};
```

**Members**

`node`
:   Node in global list of switches

`route_table`
:   Copy of switch routing table

`port_ok`
:   Status of each port (one bit per port) - OK=1 or UNINIT=0

`ops`
:   pointer to switch-specific operations

`lock`
:   lock to serialize operations updates

`nextdev`
:   Array of per-port pointers to the next attached device

struct rio\_switch\_ops
:   Per-switch operations

**Definition**:

```
struct rio_switch_ops {
    struct module *owner;
    int (*add_entry) (struct rio_mport *mport, u16 destid, u8 hopcount, u16 table, u16 route_destid, u8 route_port);
    int (*get_entry) (struct rio_mport *mport, u16 destid, u8 hopcount, u16 table, u16 route_destid, u8 *route_port);
    int (*clr_table) (struct rio_mport *mport, u16 destid, u8 hopcount, u16 table);
    int (*set_domain) (struct rio_mport *mport, u16 destid, u8 hopcount, u8 sw_domain);
    int (*get_domain) (struct rio_mport *mport, u16 destid, u8 hopcount, u8 *sw_domain);
    int (*em_init) (struct rio_dev *dev);
    int (*em_handle) (struct rio_dev *dev, u8 swport);
};
```

**Members**

`owner`
:   The module owner of this structure

`add_entry`
:   Callback for switch-specific route add function

`get_entry`
:   Callback for switch-specific route get function

`clr_table`
:   Callback for switch-specific clear route table function

`set_domain`
:   Callback for switch-specific domain setting function

`get_domain`
:   Callback for switch-specific domain get function

`em_init`
:   Callback for switch-specific error management init function

`em_handle`
:   Callback for switch-specific error management handler function

**Description**

Defines the operations that are necessary to initialize/control
a particular RIO switch device.

struct rio\_dev
:   RIO device info

**Definition**:

```
struct rio_dev {
    struct list_head global_list;
    struct list_head net_list;
    struct rio_net *net;
    bool do_enum;
    u16 did;
    u16 vid;
    u32 device_rev;
    u16 asm_did;
    u16 asm_vid;
    u16 asm_rev;
    u16 efptr;
    u32 pef;
    u32 swpinfo;
    u32 src_ops;
    u32 dst_ops;
    u32 comp_tag;
    u32 phys_efptr;
    u32 phys_rmap;
    u32 em_efptr;
    u64 dma_mask;
    struct rio_driver *driver;
    struct device dev;
    struct resource riores[RIO_MAX_DEV_RESOURCES];
    int (*pwcback) (struct rio_dev *rdev, union rio_pw_msg *msg, int step);
    u16 destid;
    u8 hopcount;
    struct rio_dev *prev;
    atomic_t state;
    struct rio_switch rswitch[];
};
```

**Members**

`global_list`
:   Node in list of all RIO devices

`net_list`
:   Node in list of RIO devices in a network

`net`
:   Network this device is a part of

`do_enum`
:   Enumeration flag

`did`
:   Device ID

`vid`
:   Vendor ID

`device_rev`
:   Device revision

`asm_did`
:   Assembly device ID

`asm_vid`
:   Assembly vendor ID

`asm_rev`
:   Assembly revision

`efptr`
:   Extended feature pointer

`pef`
:   Processing element features

`swpinfo`
:   Switch port info

`src_ops`
:   Source operation capabilities

`dst_ops`
:   Destination operation capabilities

`comp_tag`
:   RIO component tag

`phys_efptr`
:   RIO device extended features pointer

`phys_rmap`
:   LP-Serial Register Map Type (1 or 2)

`em_efptr`
:   RIO Error Management features pointer

`dma_mask`
:   Mask of bits of RIO address this device implements

`driver`
:   Driver claiming this device

`dev`
:   Device model device

`riores`
:   RIO resources this device owns

`pwcback`
:   port-write callback function for this device

`destid`
:   Network destination ID (or associated destid for switch)

`hopcount`
:   Hopcount to this device

`prev`
:   Previous RIO device connected to the current one

`state`
:   device state

`rswitch`
:   [`struct rio_switch`](#c.rio_switch "rio_switch") (if valid for this device)

struct rio\_msg
:   RIO message event

**Definition**:

```
struct rio_msg {
    struct resource *res;
    void (*mcback) (struct rio_mport * mport, void *dev_id, int mbox, int slot);
};
```

**Members**

`res`
:   Mailbox resource

`mcback`
:   Message event callback

struct rio\_dbell
:   RIO doorbell event

**Definition**:

```
struct rio_dbell {
    struct list_head node;
    struct resource *res;
    void (*dinb) (struct rio_mport *mport, void *dev_id, u16 src, u16 dst, u16 info);
    void *dev_id;
};
```

**Members**

`node`
:   Node in list of doorbell events

`res`
:   Doorbell resource

`dinb`
:   Doorbell event callback

`dev_id`
:   Device specific pointer to pass on event

struct rio\_mport
:   RIO master port info

**Definition**:

```
struct rio_mport {
    struct list_head dbells;
    struct list_head pwrites;
    struct list_head node;
    struct list_head nnode;
    struct rio_net *net;
    struct mutex lock;
    struct resource iores;
    struct resource riores[RIO_MAX_MPORT_RESOURCES];
    struct rio_msg inb_msg[RIO_MAX_MBOX];
    struct rio_msg outb_msg[RIO_MAX_MBOX];
    int host_deviceid;
    struct rio_ops *ops;
    unsigned char id;
    unsigned char index;
    unsigned int sys_size;
    u32 phys_efptr;
    u32 phys_rmap;
    unsigned char name[RIO_MAX_MPORT_NAME];
    struct device dev;
    void *priv;
#ifdef CONFIG_RAPIDIO_DMA_ENGINE;
    struct dma_device       dma;
#endif;
    struct rio_scan *nscan;
    atomic_t state;
    unsigned int pwe_refcnt;
};
```

**Members**

`dbells`
:   List of doorbell events

`pwrites`
:   List of portwrite events

`node`
:   Node in global list of master ports

`nnode`
:   Node in network list of master ports

`net`
:   RIO net this mport is attached to

`lock`
:   lock to synchronize lists manipulations

`iores`
:   I/O mem resource that this master port interface owns

`riores`
:   RIO resources that this master port interfaces owns

`inb_msg`
:   RIO inbound message event descriptors

`outb_msg`
:   RIO outbound message event descriptors

`host_deviceid`
:   Host device ID associated with this master port

`ops`
:   configuration space functions

`id`
:   Port ID, unique among all ports

`index`
:   Port index, unique among all port interfaces of the same type

`sys_size`
:   RapidIO common transport system size

`phys_efptr`
:   RIO port extended features pointer

`phys_rmap`
:   LP-Serial EFB Register Mapping type (1 or 2).

`name`
:   Port name string

`dev`
:   device structure associated with an mport

`priv`
:   Master port private data

`dma`
:   DMA device associated with mport

`nscan`
:   RapidIO network enumeration/discovery operations

`state`
:   mport device state

`pwe_refcnt`
:   port-write enable ref counter to track enable/disable requests

struct rio\_net
:   RIO network info

**Definition**:

```
struct rio_net {
    struct list_head node;
    struct list_head devices;
    struct list_head switches;
    struct list_head mports;
    struct rio_mport *hport;
    unsigned char id;
    struct device dev;
    void *enum_data;
    void (*release)(struct rio_net *net);
};
```

**Members**

`node`
:   Node in global list of RIO networks

`devices`
:   List of devices in this network

`switches`
:   List of switches in this network

`mports`
:   List of master ports accessing this network

`hport`
:   Default port for accessing this network

`id`
:   RIO network ID

`dev`
:   Device object

`enum_data`
:   private data specific to a network enumerator

`release`
:   enumerator-specific release callback

struct rio\_mport\_attr
:   RIO mport device attributes

**Definition**:

```
struct rio_mport_attr {
    int flags;
    int link_speed;
    int link_width;
    int dma_max_sge;
    int dma_max_size;
    int dma_align;
};
```

**Members**

`flags`
:   mport device capability flags

`link_speed`
:   SRIO link speed value (as defined by RapidIO specification)

`link_width`
:   SRIO link width value (as defined by RapidIO specification)

`dma_max_sge`
:   number of SG list entries that can be handled by DMA channel(s)

`dma_max_size`
:   max number of bytes in single DMA transfer (SG entry)

`dma_align`
:   alignment shift for DMA operations (as for other DMA operations)

struct rio\_ops
:   Low-level RIO configuration space operations

**Definition**:

```
struct rio_ops {
    int (*lcread) (struct rio_mport *mport, int index, u32 offset, int len, u32 *data);
    int (*lcwrite) (struct rio_mport *mport, int index, u32 offset, int len, u32 data);
    int (*cread) (struct rio_mport *mport, int index, u16 destid, u8 hopcount, u32 offset, int len, u32 *data);
    int (*cwrite) (struct rio_mport *mport, int index, u16 destid, u8 hopcount, u32 offset, int len, u32 data);
    int (*dsend) (struct rio_mport *mport, int index, u16 destid, u16 data);
    int (*pwenable) (struct rio_mport *mport, int enable);
    int (*open_outb_mbox)(struct rio_mport *mport, void *dev_id, int mbox, int entries);
    void (*close_outb_mbox)(struct rio_mport *mport, int mbox);
    int (*open_inb_mbox)(struct rio_mport *mport, void *dev_id, int mbox, int entries);
    void (*close_inb_mbox)(struct rio_mport *mport, int mbox);
    int (*add_outb_message)(struct rio_mport *mport, struct rio_dev *rdev, int mbox, void *buffer, size_t len);
    int (*add_inb_buffer)(struct rio_mport *mport, int mbox, void *buf);
    void *(*get_inb_message)(struct rio_mport *mport, int mbox);
    int (*map_inb)(struct rio_mport *mport, dma_addr_t lstart, u64 rstart, u64 size, u32 flags);
    void (*unmap_inb)(struct rio_mport *mport, dma_addr_t lstart);
    int (*query_mport)(struct rio_mport *mport, struct rio_mport_attr *attr);
    int (*map_outb)(struct rio_mport *mport, u16 destid, u64 rstart, u32 size, u32 flags, dma_addr_t *laddr);
    void (*unmap_outb)(struct rio_mport *mport, u16 destid, u64 rstart);
};
```

**Members**

`lcread`
:   Callback to perform local (master port) read of config space.

`lcwrite`
:   Callback to perform local (master port) write of config space.

`cread`
:   Callback to perform network read of config space.

`cwrite`
:   Callback to perform network write of config space.

`dsend`
:   Callback to send a doorbell message.

`pwenable`
:   Callback to enable/disable port-write message handling.

`open_outb_mbox`
:   Callback to initialize outbound mailbox.

`close_outb_mbox`
:   Callback to shut down outbound mailbox.

`open_inb_mbox`
:   Callback to initialize inbound mailbox.

`close_inb_mbox`
:   Callback to shut down inbound mailbox.

`add_outb_message`
:   Callback to add a message to an outbound mailbox queue.

`add_inb_buffer`
:   Callback to add a buffer to an inbound mailbox queue.

`get_inb_message`
:   Callback to get a message from an inbound mailbox queue.

`map_inb`
:   Callback to map RapidIO address region into local memory space.

`unmap_inb`
:   Callback to unmap RapidIO address region mapped with `map_inb()`.

`query_mport`
:   Callback to query mport device attributes.

`map_outb`
:   Callback to map outbound address region into local memory space.

`unmap_outb`
:   Callback to unmap outbound RapidIO address region.

struct rio\_driver
:   RIO driver info

**Definition**:

```
struct rio_driver {
    struct list_head node;
    char *name;
    const struct rio_device_id *id_table;
    int (*probe) (struct rio_dev * dev, const struct rio_device_id * id);
    void (*remove) (struct rio_dev * dev);
    void (*shutdown)(struct rio_dev *dev);
    int (*suspend) (struct rio_dev * dev, u32 state);
    int (*resume) (struct rio_dev * dev);
    int (*enable_wake) (struct rio_dev * dev, u32 state, int enable);
    struct device_driver driver;
};
```

**Members**

`node`
:   Node in list of drivers

`name`
:   RIO driver name

`id_table`
:   RIO device ids to be associated with this driver

`probe`
:   RIO device inserted

`remove`
:   RIO device removed

`shutdown`
:   shutdown notification callback

`suspend`
:   RIO device suspended

`resume`
:   RIO device awakened

`enable_wake`
:   RIO device enable wake event

`driver`
:   LDM driver struct

**Description**

Provides info on a RIO device driver for insertion/removal and
power management purposes.

struct rio\_scan
:   RIO enumeration and discovery operations

**Definition**:

```
struct rio_scan {
    struct module *owner;
    int (*enumerate)(struct rio_mport *mport, u32 flags);
    int (*discover)(struct rio_mport *mport, u32 flags);
};
```

**Members**

`owner`
:   The module owner of this structure

`enumerate`
:   Callback to perform RapidIO fabric enumeration.

`discover`
:   Callback to perform RapidIO fabric discovery.

struct rio\_scan\_node
:   list node to register RapidIO enumeration and discovery methods with RapidIO core.

**Definition**:

```
struct rio_scan_node {
    int mport_id;
    struct list_head node;
    struct rio_scan *ops;
};
```

**Members**

`mport_id`
:   ID of an mport (net) serviced by this enumerator

`node`
:   node in global list of registered enumerators

`ops`
:   RIO enumeration and discovery operations

### Enumeration and Discovery

u16 rio\_destid\_alloc(struct [rio\_net](#c.rio_net "rio_net") \*net)
:   Allocate next available destID for given network

**Parameters**

`struct rio_net *net`
:   RIO network

**Description**

Returns next available device destination ID for the specified RIO network.
Marks allocated ID as one in use.
Returns RIO\_INVALID\_DESTID if new destID is not available.

int rio\_destid\_reserve(struct [rio\_net](#c.rio_net "rio_net") \*net, u16 destid)
:   Reserve the specified destID

**Parameters**

`struct rio_net *net`
:   RIO network

`u16 destid`
:   destID to reserve

**Description**

Tries to reserve the specified destID.
Returns 0 if successful.

void rio\_destid\_free(struct [rio\_net](#c.rio_net "rio_net") \*net, u16 destid)
:   free a previously allocated destID

**Parameters**

`struct rio_net *net`
:   RIO network

`u16 destid`
:   destID to free

**Description**

Makes the specified destID available for use.

u16 rio\_destid\_first(struct [rio\_net](#c.rio_net "rio_net") \*net)
:   return first destID in use

**Parameters**

`struct rio_net *net`
:   RIO network

u16 rio\_destid\_next(struct [rio\_net](#c.rio_net "rio_net") \*net, u16 from)
:   return next destID in use

**Parameters**

`struct rio_net *net`
:   RIO network

`u16 from`
:   destination ID from which search shall continue

u16 rio\_get\_device\_id(struct [rio\_mport](#c.rio_mport "rio_mport") \*port, u16 destid, u8 hopcount)
:   Get the base/extended device id for a device

**Parameters**

`struct rio_mport *port`
:   RIO master port

`u16 destid`
:   Destination ID of device

`u8 hopcount`
:   Hopcount to device

**Description**

Reads the base/extended device id from a device. Returns the
8/16-bit device ID.

void rio\_set\_device\_id(struct [rio\_mport](#c.rio_mport "rio_mport") \*port, u16 destid, u8 hopcount, u16 did)
:   Set the base/extended device id for a device

**Parameters**

`struct rio_mport *port`
:   RIO master port

`u16 destid`
:   Destination ID of device

`u8 hopcount`
:   Hopcount to device

`u16 did`
:   Device ID value to be written

**Description**

Writes the base/extended device id from a device.

int rio\_clear\_locks(struct [rio\_net](#c.rio_net "rio_net") \*net)
:   Release all host locks and signal enumeration complete

**Parameters**

`struct rio_net *net`
:   RIO network to run on

**Description**

Marks the component tag CSR on each device with the enumeration
complete flag. When complete, it then release the host locks on
each device. Returns 0 on success or `-EINVAL` on failure.

int rio\_enum\_host(struct [rio\_mport](#c.rio_mport "rio_mport") \*port)
:   Set host lock and initialize host destination ID

**Parameters**

`struct rio_mport *port`
:   Master port to issue transaction

**Description**

Sets the local host master port lock and destination ID register
with the host device ID value. The host device ID value is provided
by the platform. Returns `0` on success or `-1` on failure.

int rio\_device\_has\_destid(struct [rio\_mport](#c.rio_mport "rio_mport") \*port, int src\_ops, int dst\_ops)
:   Test if a device contains a destination ID register

**Parameters**

`struct rio_mport *port`
:   Master port to issue transaction

`int src_ops`
:   RIO device source operations

`int dst_ops`
:   RIO device destination operations

**Description**

Checks the provided **src\_ops** and **dst\_ops** for the necessary transaction
capabilities that indicate whether or not a device will implement a
destination ID register. Returns 1 if true or 0 if false.

void rio\_release\_dev(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev)
:   Frees a RIO device struct

**Parameters**

`struct device *dev`
:   LDM device associated with a RIO device struct

**Description**

Gets the RIO device `struct associated` a RIO device struct.
The RIO device `struct is` freed.

int rio\_is\_switch(struct [rio\_dev](#c.rio_dev "rio_dev") \*rdev)
:   Tests if a RIO device has switch capabilities

**Parameters**

`struct rio_dev *rdev`
:   RIO device

**Description**

Gets the RIO device Processing Element Features register
contents and tests for switch capabilities. Returns 1 if
the device is a switch or 0 if it is not a switch.
The RIO device `struct is` freed.

struct [rio\_dev](#c.rio_dev "rio_dev") \*rio\_setup\_device(struct [rio\_net](#c.rio_net "rio_net") \*net, struct [rio\_mport](#c.rio_mport "rio_mport") \*port, u16 destid, u8 hopcount, int do\_enum)
:   Allocates and sets up a RIO device

**Parameters**

`struct rio_net *net`
:   RIO network

`struct rio_mport *port`
:   Master port to send transactions

`u16 destid`
:   Current destination ID

`u8 hopcount`
:   Current hopcount

`int do_enum`
:   Enumeration/Discovery mode flag

**Description**

Allocates a RIO device and configures fields based on configuration
space contents. If device has a destination ID register, a destination
ID is either assigned in enumeration mode or read from configuration
space in discovery mode. If the device has switch capabilities, then
a switch is allocated and configured appropriately. Returns a pointer
to a RIO device on success or NULL on failure.

int rio\_sport\_is\_active(struct [rio\_dev](#c.rio_dev "rio_dev") \*rdev, int sp)
:   Tests if a switch port has an active connection.

**Parameters**

`struct rio_dev *rdev`
:   RapidIO device object

`int sp`
:   Switch port number

**Description**

Reads the port error status CSR for a particular switch port to
determine if the port has an active link. Returns
`RIO_PORT_N_ERR_STS_PORT_OK` if the port is active or `0` if it is
inactive.

u16 rio\_get\_host\_deviceid\_lock(struct [rio\_mport](#c.rio_mport "rio_mport") \*port, u8 hopcount)
:   Reads the Host Device ID Lock CSR on a device

**Parameters**

`struct rio_mport *port`
:   Master port to send transaction

`u8 hopcount`
:   Number of hops to the device

**Description**

Used during enumeration to read the Host Device ID Lock CSR on a
RIO device. Returns the value of the lock register.

int rio\_enum\_peer(struct [rio\_net](#c.rio_net "rio_net") \*net, struct [rio\_mport](#c.rio_mport "rio_mport") \*port, u8 hopcount, struct [rio\_dev](#c.rio_dev "rio_dev") \*prev, int prev\_port)
:   Recursively enumerate a RIO network through a master port

**Parameters**

`struct rio_net *net`
:   RIO network being enumerated

`struct rio_mport *port`
:   Master port to send transactions

`u8 hopcount`
:   Number of hops into the network

`struct rio_dev *prev`
:   Previous RIO device connected to the enumerated one

`int prev_port`
:   Port on previous RIO device

**Description**

Recursively enumerates a RIO network. Transactions are sent via the
master port passed in **port**.

int rio\_enum\_complete(struct [rio\_mport](#c.rio_mport "rio_mport") \*port)
:   Tests if enumeration of a network is complete

**Parameters**

`struct rio_mport *port`
:   Master port to send transaction

**Description**

Tests the PGCCSR discovered bit for non-zero value (enumeration
complete flag). Return `1` if enumeration is complete or `0` if
enumeration is incomplete.

int rio\_disc\_peer(struct [rio\_net](#c.rio_net "rio_net") \*net, struct [rio\_mport](#c.rio_mport "rio_mport") \*port, u16 destid, u8 hopcount, struct [rio\_dev](#c.rio_dev "rio_dev") \*prev, int prev\_port)
:   Recursively discovers a RIO network through a master port

**Parameters**

`struct rio_net *net`
:   RIO network being discovered

`struct rio_mport *port`
:   Master port to send transactions

`u16 destid`
:   Current destination ID in network

`u8 hopcount`
:   Number of hops into the network

`struct rio_dev *prev`
:   previous rio\_dev

`int prev_port`
:   previous port number

**Description**

Recursively discovers a RIO network. Transactions are sent via the
master port passed in **port**.

int rio\_mport\_is\_active(struct [rio\_mport](#c.rio_mport "rio_mport") \*port)
:   Tests if master port link is active

**Parameters**

`struct rio_mport *port`
:   Master port to test

**Description**

Reads the port error status CSR for the master port to
determine if the port has an active link. Returns
`RIO_PORT_N_ERR_STS_PORT_OK` if the master port is active
or `0` if it is inactive.

void rio\_update\_route\_tables(struct [rio\_net](#c.rio_net "rio_net") \*net)
:   Updates route tables in switches

**Parameters**

`struct rio_net *net`
:   RIO network to run update on

**Description**

For each enumerated device, ensure that each switch in a system
has correct routing entries. Add routes for devices that where
unknown during the first enumeration pass through the switch.

void rio\_init\_em(struct [rio\_dev](#c.rio_dev "rio_dev") \*rdev)
:   Initializes RIO Error Management (for switches)

**Parameters**

`struct rio_dev *rdev`
:   RIO device

**Description**

For each enumerated switch, call device-specific error management
initialization routine (if supplied by the switch driver).

int rio\_enum\_mport(struct [rio\_mport](#c.rio_mport "rio_mport") \*mport, u32 flags)
:   Start enumeration through a master port

**Parameters**

`struct rio_mport *mport`
:   Master port to send transactions

`u32 flags`
:   Enumeration control flags

**Description**

Starts the enumeration process. If somebody has enumerated our
master port device, then give up. If not and we have an active
link, then start recursive peer enumeration. Returns `0` if
enumeration succeeds or `-EBUSY` if enumeration fails.

void rio\_build\_route\_tables(struct [rio\_net](#c.rio_net "rio_net") \*net)
:   Generate route tables from switch route entries

**Parameters**

`struct rio_net *net`
:   RIO network to run route tables scan on

**Description**

For each switch device, generate a route table by copying existing
route entries from the switch.

int rio\_disc\_mport(struct [rio\_mport](#c.rio_mport "rio_mport") \*mport, u32 flags)
:   Start discovery through a master port

**Parameters**

`struct rio_mport *mport`
:   Master port to send transactions

`u32 flags`
:   discovery control flags

**Description**

Starts the discovery process. If we have an active link,
then wait for the signal that enumeration is complete (if wait
is allowed).
When enumeration completion is signaled, start recursive
peer discovery. Returns `0` if discovery succeeds or `-EBUSY`
on failure.

int rio\_basic\_attach(void)

**Parameters**

`void`
:   no arguments

**Description**

When this enumeration/discovery method is loaded as a module this function
registers its specific enumeration and discover routines for all available
RapidIO mport devices. The “scan” command line parameter controls ability of
the module to start RapidIO enumeration/discovery automatically.

Returns 0 for success or -EIO if unable to register itself.

This enumeration/discovery method cannot be unloaded and therefore does not
provide a matching cleanup\_module routine.

### Driver functionality

int rio\_setup\_inb\_dbell(struct [rio\_mport](#c.rio_mport "rio_mport") \*mport, void \*dev\_id, struct resource \*res, void (\*dinb)(struct [rio\_mport](#c.rio_mport "rio_mport") \*mport, void \*dev\_id, u16 src, u16 dst, u16 info))
:   bind inbound doorbell callback

**Parameters**

`struct rio_mport *mport`
:   RIO master port to bind the doorbell callback

`void *dev_id`
:   Device specific pointer to pass on event

`struct resource *res`
:   Doorbell message resource

`void (*dinb) (struct rio_mport * mport, void *dev_id, u16 src, u16 dst, u16 info)`
:   Callback to execute when doorbell is received

**Description**

Adds a doorbell resource/callback pair into a port’s
doorbell event list. Returns 0 if the request has been
satisfied.

int rio\_chk\_dev\_route(struct [rio\_dev](#c.rio_dev "rio_dev") \*rdev, struct [rio\_dev](#c.rio_dev "rio_dev") \*\*nrdev, int \*npnum)
:   Validate route to the specified device.

**Parameters**

`struct rio_dev *rdev`
:   RIO device failed to respond

`struct rio_dev **nrdev`
:   Last active device on the route to rdev

`int *npnum`
:   nrdev’s port number on the route to rdev

**Description**

Follows a route to the specified RIO device to determine the last available
device (and corresponding RIO port) on the route.

int rio\_chk\_dev\_access(struct [rio\_dev](#c.rio_dev "rio_dev") \*rdev)
:   Validate access to the specified device.

**Parameters**

`struct rio_dev *rdev`
:   Pointer to RIO device control structure

int rio\_get\_input\_status(struct [rio\_dev](#c.rio_dev "rio_dev") \*rdev, int pnum, u32 \*lnkresp)
:   Sends a Link-Request/Input-Status control symbol and returns link-response (if requested).

**Parameters**

`struct rio_dev *rdev`
:   RIO devive to issue Input-status command

`int pnum`
:   Device port number to issue the command

`u32 *lnkresp`
:   Response from a link partner

int rio\_clr\_err\_stopped(struct [rio\_dev](#c.rio_dev "rio_dev") \*rdev, u32 pnum, u32 err\_status)
:   Clears port Error-stopped states.

**Parameters**

`struct rio_dev *rdev`
:   Pointer to RIO device control structure

`u32 pnum`
:   Switch port number to clear errors

`u32 err_status`
:   port error status (if 0 reads register from device)

**Description**

TODO: Currently this routine is not compatible with recovery process
specified for idt\_gen3 RapidIO switch devices. It has to be reviewed
to implement universal recovery process that is compatible full range
off available devices.
IDT gen3 switch driver now implements HW-specific error handler that
issues soft port reset to the port to reset ERR\_STOP bits and ackIDs.

int rio\_std\_route\_add\_entry(struct [rio\_mport](#c.rio_mport "rio_mport") \*mport, u16 destid, u8 hopcount, u16 table, u16 route\_destid, u8 route\_port)
:   Add switch route table entry using standard registers defined in RIO specification rev.1.3

**Parameters**

`struct rio_mport *mport`
:   Master port to issue transaction

`u16 destid`
:   Destination ID of the device

`u8 hopcount`
:   Number of switch hops to the device

`u16 table`
:   routing table ID (global or port-specific)

`u16 route_destid`
:   destID entry in the RT

`u8 route_port`
:   destination port for specified destID

int rio\_std\_route\_get\_entry(struct [rio\_mport](#c.rio_mport "rio_mport") \*mport, u16 destid, u8 hopcount, u16 table, u16 route\_destid, u8 \*route\_port)
:   Read switch route table entry (port number) associated with specified destID using standard registers defined in RIO specification rev.1.3

**Parameters**

`struct rio_mport *mport`
:   Master port to issue transaction

`u16 destid`
:   Destination ID of the device

`u8 hopcount`
:   Number of switch hops to the device

`u16 table`
:   routing table ID (global or port-specific)

`u16 route_destid`
:   destID entry in the RT

`u8 *route_port`
:   returned destination port for specified destID

int rio\_std\_route\_clr\_table(struct [rio\_mport](#c.rio_mport "rio_mport") \*mport, u16 destid, u8 hopcount, u16 table)
:   Clear swotch route table using standard registers defined in RIO specification rev.1.3.

**Parameters**

`struct rio_mport *mport`
:   Master port to issue transaction

`u16 destid`
:   Destination ID of the device

`u8 hopcount`
:   Number of switch hops to the device

`u16 table`
:   routing table ID (global or port-specific)

int rio\_mport\_scan(int mport\_id)
:   execute enumeration/discovery on the specified mport

**Parameters**

`int mport_id`
:   number (ID) of mport device

RIO\_LOP\_READ

`RIO_LOP_READ (size, type, len)`

> Generate rio\_local\_read\_config\_\* functions

**Parameters**

`size`
:   Size of configuration space read (8, 16, 32 bits)

`type`
:   C type of value argument

`len`
:   Length of configuration space read (1, 2, 4 bytes)

**Description**

Generates rio\_local\_read\_config\_\* functions used to access
configuration space registers on the local device.

RIO\_LOP\_WRITE

`RIO_LOP_WRITE (size, type, len)`

> Generate rio\_local\_write\_config\_\* functions

**Parameters**

`size`
:   Size of configuration space write (8, 16, 32 bits)

`type`
:   C type of value argument

`len`
:   Length of configuration space write (1, 2, 4 bytes)

**Description**

Generates rio\_local\_write\_config\_\* functions used to access
configuration space registers on the local device.

RIO\_OP\_READ

`RIO_OP_READ (size, type, len)`

> Generate rio\_mport\_read\_config\_\* functions

**Parameters**

`size`
:   Size of configuration space read (8, 16, 32 bits)

`type`
:   C type of value argument

`len`
:   Length of configuration space read (1, 2, 4 bytes)

**Description**

Generates rio\_mport\_read\_config\_\* functions used to access
configuration space registers on the local device.

RIO\_OP\_WRITE

`RIO_OP_WRITE (size, type, len)`

> Generate rio\_mport\_write\_config\_\* functions

**Parameters**

`size`
:   Size of configuration space write (8, 16, 32 bits)

`type`
:   C type of value argument

`len`
:   Length of configuration space write (1, 2, 4 bytes)

**Description**

Generates rio\_mport\_write\_config\_\* functions used to access
configuration space registers on the local device.

### Device model support

const struct [rio\_device\_id](../driver-api/basics.html#c.rio_device_id "rio_device_id") \*rio\_match\_device(const struct [rio\_device\_id](../driver-api/basics.html#c.rio_device_id "rio_device_id") \*id, const struct [rio\_dev](#c.rio_dev "rio_dev") \*rdev)
:   Tell if a RIO device has a matching RIO device id structure

**Parameters**

`const struct rio_device_id *id`
:   the RIO device id structure to match against

`const struct rio_dev *rdev`
:   the RIO device structure to match against

**Description**

> Used from driver probe and bus matching to check whether a RIO device
> matches a device id structure provided by a RIO driver. Returns the
> matching [`struct rio_device_id`](../driver-api/basics.html#c.rio_device_id "rio_device_id") or `NULL` if there is no match.

int rio\_device\_probe(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev)
:   Tell if a RIO device structure has a matching RIO device id structure

**Parameters**

`struct device *dev`
:   the RIO device structure to match against

**Description**

return 0 and set rio\_dev->driver when drv claims rio\_dev, else error

void rio\_device\_remove(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev)
:   Remove a RIO device from the system

**Parameters**

`struct device *dev`
:   the RIO device structure to match against

**Description**

Remove a RIO device from the system. If it has an associated
driver, then run the driver `remove()` method. Then update
the reference count.

int rio\_match\_bus(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, const struct [device\_driver](../driver-api/infrastructure.html#c.device_driver "device_driver") \*drv)
:   Tell if a RIO device structure has a matching RIO driver device id structure

**Parameters**

`struct device *dev`
:   the standard device structure to match against

`const struct device_driver *drv`
:   the standard driver structure containing the ids to match against

**Description**

> Used by a driver to check whether a RIO device present in the
> system is in its list of supported devices. Returns 1 if
> there is a matching [`struct rio_device_id`](../driver-api/basics.html#c.rio_device_id "rio_device_id") or 0 if there is
> no match.

int rio\_bus\_init(void)
:   Register the RapidIO bus with the device model

**Parameters**

`void`
:   no arguments

**Description**

> Registers the RIO mport device class and RIO bus type with the Linux
> device model.

### PPC32 support

int fsl\_local\_config\_read(struct [rio\_mport](#c.rio_mport "rio_mport") \*mport, int index, u32 offset, int len, u32 \*data)
:   Generate a MPC85xx local config space read

**Parameters**

`struct rio_mport *mport`
:   RapidIO master port info

`int index`
:   ID of RapdiIO interface

`u32 offset`
:   Offset into configuration space

`int len`
:   Length (in bytes) of the maintenance transaction

`u32 *data`
:   Value to be read into

**Description**

Generates a MPC85xx local configuration space read. Returns `0` on
success or `-EINVAL` on failure.

int fsl\_local\_config\_write(struct [rio\_mport](#c.rio_mport "rio_mport") \*mport, int index, u32 offset, int len, u32 data)
:   Generate a MPC85xx local config space write

**Parameters**

`struct rio_mport *mport`
:   RapidIO master port info

`int index`
:   ID of RapdiIO interface

`u32 offset`
:   Offset into configuration space

`int len`
:   Length (in bytes) of the maintenance transaction

`u32 data`
:   Value to be written

**Description**

Generates a MPC85xx local configuration space write. Returns `0` on
success or `-EINVAL` on failure.

int fsl\_rio\_config\_read(struct [rio\_mport](#c.rio_mport "rio_mport") \*mport, int index, u16 destid, u8 hopcount, u32 offset, int len, u32 \*val)
:   Generate a MPC85xx read maintenance transaction

**Parameters**

`struct rio_mport *mport`
:   RapidIO master port info

`int index`
:   ID of RapdiIO interface

`u16 destid`
:   Destination ID of transaction

`u8 hopcount`
:   Number of hops to target device

`u32 offset`
:   Offset into configuration space

`int len`
:   Length (in bytes) of the maintenance transaction

`u32 *val`
:   Location to be read into

**Description**

Generates a MPC85xx read maintenance transaction. Returns `0` on
success or `-EINVAL` on failure.

int fsl\_rio\_config\_write(struct [rio\_mport](#c.rio_mport "rio_mport") \*mport, int index, u16 destid, u8 hopcount, u32 offset, int len, u32 val)
:   Generate a MPC85xx write maintenance transaction

**Parameters**

`struct rio_mport *mport`
:   RapidIO master port info

`int index`
:   ID of RapdiIO interface

`u16 destid`
:   Destination ID of transaction

`u8 hopcount`
:   Number of hops to target device

`u32 offset`
:   Offset into configuration space

`int len`
:   Length (in bytes) of the maintenance transaction

`u32 val`
:   Value to be written

**Description**

Generates an MPC85xx write maintenance transaction. Returns `0` on
success or `-EINVAL` on failure.

int fsl\_rio\_setup(struct platform\_device \*dev)
:   Setup Freescale PowerPC RapidIO interface

**Parameters**

`struct platform_device *dev`
:   platform\_device pointer

**Description**

Initializes MPC85xx RapidIO hardware interface, configures
master port with system-specific info, and registers the
master port with the RapidIO subsystem.

## Credits

The following people have contributed to the RapidIO subsystem directly
or indirectly:

1. Matt Porter[mporter@kernel.crashing.org](mailto:mporter%40kernel.crashing.org)
2. Randy Vinson[rvinson@mvista.com](mailto:rvinson%40mvista.com)
3. Dan Malek[dan@embeddedalley.com](mailto:dan%40embeddedalley.com)

The following people have contributed to this document:

1. Matt Porter[mporter@kernel.crashing.org](mailto:mporter%40kernel.crashing.org)
