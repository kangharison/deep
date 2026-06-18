# PCI Peer-to-Peer DMA Support

> 출처(원문): https://docs.kernel.org/driver-api/pci/p2pdma.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# PCI Peer-to-Peer DMA Support

The PCI bus has pretty decent support for performing DMA transfers
between two devices on the bus. This type of transaction is henceforth
called Peer-to-Peer (or P2P). However, there are a number of issues that
make P2P transactions tricky to do in a perfectly safe way.

For PCIe the routing of Transaction Layer Packets (TLPs) is well-defined up
until they reach a host bridge or root port. If the path includes PCIe switches
then based on the ACS settings the transaction can route entirely within
the PCIe hierarchy and never reach the root port. The kernel will evaluate
the PCIe topology and always permit P2P in these well-defined cases.

However, if the P2P transaction reaches the host bridge then it might have to
hairpin back out the same root port, be routed inside the CPU SOC to another
PCIe root port, or routed internally to the SOC.

The PCIe specification doesn’t define the forwarding of transactions between
hierarchy domains and kernel defaults to blocking such routing. There is an
allow list to allow detecting known-good HW, in which case P2P between any
two PCIe devices will be permitted.

Since P2P inherently is doing transactions between two devices it requires two
drivers to be co-operating inside the kernel. The providing driver has to convey
its MMIO to the consuming driver. To meet the driver model lifecycle rules the
MMIO must have all DMA mapping removed, all CPU accesses prevented, all page
table mappings undone before the providing driver completes `remove()`.

This requires the providing and consuming driver to actively work together to
guarantee that the consuming driver has stopped using the MMIO during a removal
cycle. This is done by either a synchronous invalidation shutdown or waiting
for all usage refcounts to reach zero.

At the lowest level the P2P subsystem offers a naked `struct p2p_provider` that
delegates lifecycle management to the providing driver. It is expected that
drivers using this option will wrap their MMIO memory in DMABUF and use DMABUF
to provide an invalidation shutdown. These MMIO addresses have no `struct page`, and
if used with mmap() must create special PTEs. As such there are very few
kernel uAPIs that can accept pointers to them; in particular they cannot be used
with read()/write(), including O\_DIRECT.

Building on this, the subsystem offers a layer to wrap the MMIO in a ZONE\_DEVICE
pgmap of MEMORY\_DEVICE\_PCI\_P2PDMA to create `struct pages`. The lifecycle of
pgmap ensures that when the pgmap is destroyed all other drivers have stopped
using the MMIO. This option works with O\_DIRECT flows, in some cases, if the
underlying subsystem supports handling MEMORY\_DEVICE\_PCI\_P2PDMA through
FOLL\_PCI\_P2PDMA. The use of FOLL\_LONGTERM is prevented. As this relies on pgmap
it also relies on architecture support along with alignment and minimum size
limitations.

## Driver Writer’s Guide

In a given P2P implementation there may be three or more different
types of kernel drivers in play:

* Provider - A driver which provides or publishes P2P resources like
  memory or doorbell registers to other drivers.
* Client - A driver which makes use of a resource by setting up a
  DMA transaction to or from it.
* Orchestrator - A driver which orchestrates the flow of data between
  clients and providers.

In many cases there could be overlap between these three types (i.e.,
it may be typical for a driver to be both a provider and a client).

For example, in the NVMe Target Copy Offload implementation:

* The NVMe PCI driver is both a client, provider and orchestrator
  in that it exposes any CMB (Controller Memory Buffer) as a P2P memory
  resource (provider), it accepts P2P memory pages as buffers in requests
  to be used directly (client) and it can also make use of the CMB as
  submission queue entries (orchestrator).
* The RDMA driver is a client in this arrangement so that an RNIC
  can DMA directly to the memory exposed by the NVMe device.
* The NVMe Target driver (nvmet) can orchestrate the data from the RNIC
  to the P2P memory (CMB) and then to the NVMe device (and vice versa).

This is currently the only arrangement supported by the kernel but
one could imagine slight tweaks to this that would allow for the same
functionality. For example, if a specific RNIC added a BAR with some
memory behind it, its driver could add support as a P2P provider and
then the NVMe Target could use the RNIC’s memory instead of the CMB
in cases where the NVMe cards in use do not have CMB support.

### Provider Drivers

A provider simply needs to register a BAR (or a portion of a BAR)
as a P2P DMA resource using [`pci_p2pdma_add_resource()`](#c.pci_p2pdma_add_resource "pci_p2pdma_add_resource").
This will register `struct pages` for all the specified memory.

After that it may optionally publish all of its resources as
P2P memory using [`pci_p2pmem_publish()`](#c.pci_p2pmem_publish "pci_p2pmem_publish"). This will allow
any orchestrator drivers to find and use the memory. When marked in
this way, the resource must be regular memory with no side effects.

For the time being this is fairly rudimentary in that all resources
are typically going to be P2P memory. Future work will likely expand
this to include other types of resources like doorbells.

### Client Drivers

A client driver only has to use the mapping API `dma_map_sg()`
and `dma_unmap_sg()` functions as usual, and the implementation
will do the right thing for the P2P capable memory.

### Orchestrator Drivers

The first task an orchestrator driver must do is compile a list of
all client devices that will be involved in a given transaction. For
example, the NVMe Target driver creates a list including the namespace
block device and the RNIC in use. If the orchestrator has access to
a specific P2P provider to use it may check compatibility using
`pci_p2pdma_distance()` otherwise it may find a memory provider
that’s compatible with all clients using `pci_p2pmem_find()`.
If more than one provider is supported, the one nearest to all the clients will
be chosen first. If more than one provider is an equal distance away, the
one returned will be chosen at random (it is not an arbitrary but
truly random). This function returns the PCI device to use for the provider
with a reference taken and therefore when it’s no longer needed it should be
returned with [`pci_dev_put()`](pci.html#c.pci_dev_put "pci_dev_put").

Once a provider is selected, the orchestrator can then use
[`pci_alloc_p2pmem()`](#c.pci_alloc_p2pmem "pci_alloc_p2pmem") and [`pci_free_p2pmem()`](#c.pci_free_p2pmem "pci_free_p2pmem") to
allocate P2P memory from the provider. [`pci_p2pmem_alloc_sgl()`](#c.pci_p2pmem_alloc_sgl "pci_p2pmem_alloc_sgl")
and [`pci_p2pmem_free_sgl()`](#c.pci_p2pmem_free_sgl "pci_p2pmem_free_sgl") are convenience functions for
allocating scatter-gather lists with P2P memory.

### Struct Page Caveats

While the MEMORY\_DEVICE\_PCI\_P2PDMA pages can be installed in VMAs,
`pin_user_pages()` and related will not return them unless FOLL\_PCI\_P2PDMA is set.

The MEMORY\_DEVICE\_PCI\_P2PDMA pages require care to support in the kernel. The
KVA is still MMIO and must still be accessed through the normal
`readX()`/`writeX()`/etc helpers. Direct CPU access (e.g. memcpy) is forbidden, just
like any other MMIO mapping. While this will actually work on some
architectures, others will experience corruption or just crash in the kernel.
Supporting FOLL\_PCI\_P2PDMA in a subsystem requires scrubbing it to ensure no CPU
access happens.

## Usage With DMABUF

DMABUF provides an alternative to the above `struct page`-based
client/provider/orchestrator system and should be used when `struct page`
doesn’t exist. In this mode the exporting driver will wrap
some of its MMIO in a DMABUF and give the DMABUF FD to userspace.

Userspace can then pass the FD to an importing driver which will ask the
exporting driver to map it to the importer.

In this case the initiator and target pci\_devices are known and the P2P subsystem
is used to determine the mapping type. The phys\_addr\_t-based DMA API is used to
establish the dma\_addr\_t.

Lifecycle is controlled by DMABUF `move_notify()`. When the exporting driver wants
to `remove()` it must deliver an invalidation shutdown to all DMABUF importing
drivers through `move_notify()` and synchronously DMA unmap all the MMIO.

No importing driver can continue to have a DMA map to the MMIO after the
exporting driver has destroyed its p2p\_provider.

## P2P DMA Support Library

int pcim\_p2pdma\_init(struct pci\_dev \*pdev)
:   Initialise peer-to-peer DMA providers

**Parameters**

`struct pci_dev *pdev`
:   The PCI device to enable P2PDMA for

**Description**

This function initializes the peer-to-peer DMA infrastructure
for a PCI device. It allocates and sets up the necessary data
structures to support P2PDMA operations, including mapping type
tracking.

struct p2pdma\_provider \*pcim\_p2pdma\_provider(struct pci\_dev \*pdev, int bar)
:   Get peer-to-peer DMA provider

**Parameters**

`struct pci_dev *pdev`
:   The PCI device to enable P2PDMA for

`int bar`
:   BAR index to get provider

**Description**

This function gets peer-to-peer DMA provider for a PCI device. The lifetime
of the provider (and of course the MMIO) is bound to the lifetime of the
driver. A driver calling this function must ensure that all references to the
provider, and any DMA mappings created for any MMIO, are all cleaned up
before the driver `remove()` completes.

Since P2P is almost always shared with a second driver this means some system
to notify, invalidate and revoke the MMIO’s DMA must be in place to use this
function. For example a revoke can be built using DMABUF.

int pci\_p2pdma\_add\_resource(struct pci\_dev \*pdev, int bar, size\_t size, u64 offset)
:   add memory for use as p2p memory

**Parameters**

`struct pci_dev *pdev`
:   the device to add the memory to

`int bar`
:   PCI BAR to add

`size_t size`
:   size of the memory to add, may be zero to use the whole BAR

`u64 offset`
:   offset into the PCI BAR

**Description**

The memory will be given ZONE\_DEVICE `struct pages` so that it may
be used with any DMA request.

int pci\_p2pdma\_distance\_many(struct pci\_dev \*provider, struct [device](../infrastructure.html#c.device "device") \*\*clients, int num\_clients, bool verbose)
:   Determine the cumulative distance between a p2pdma provider and the clients in use.

**Parameters**

`struct pci_dev *provider`
:   p2pdma provider to check against the client list

`struct device **clients`
:   array of devices to check (NULL-terminated)

`int num_clients`
:   number of clients in the array

`bool verbose`
:   if true, print warnings for devices when we return -1

**Description**

Returns -1 if any of the clients are not compatible, otherwise returns a
positive number where a lower number is the preferable choice. (If there’s
one client that’s the same as the provider it will return 0, which is best
choice).

“compatible” means the provider and the clients are either all behind
the same PCI root port or the host bridges connected to each of the devices
are listed in the ‘pci\_p2pdma\_whitelist’.

struct pci\_dev \*pci\_p2pmem\_find\_many(struct [device](../infrastructure.html#c.device "device") \*\*clients, int num\_clients)
:   find a peer-to-peer DMA memory device compatible with the specified list of clients and shortest distance

**Parameters**

`struct device **clients`
:   array of devices to check (NULL-terminated)

`int num_clients`
:   number of client devices in the list

**Description**

If multiple devices are behind the same switch, the one “closest” to the
client devices in use will be chosen first. (So if one of the providers is
the same as one of the clients, that provider will be used ahead of any
other providers that are unrelated). If multiple providers are an equal
distance away, one will be chosen at random.

Returns a pointer to the PCI device with a reference taken (use pci\_dev\_put
to return the reference) or NULL if no compatible device is found. The
found provider will also be assigned to the client list.

void \*pci\_alloc\_p2pmem(struct pci\_dev \*pdev, size\_t size)
:   allocate peer-to-peer DMA memory

**Parameters**

`struct pci_dev *pdev`
:   the device to allocate memory from

`size_t size`
:   number of bytes to allocate

**Description**

Returns the allocated memory or NULL on error.

void pci\_free\_p2pmem(struct pci\_dev \*pdev, void \*addr, size\_t size)
:   free peer-to-peer DMA memory

**Parameters**

`struct pci_dev *pdev`
:   the device the memory was allocated from

`void *addr`
:   address of the memory that was allocated

`size_t size`
:   number of bytes that were allocated

pci\_bus\_addr\_t pci\_p2pmem\_virt\_to\_bus(struct pci\_dev \*pdev, void \*addr)
:   return the PCI bus address for a given virtual address obtained with [`pci_alloc_p2pmem()`](#c.pci_alloc_p2pmem "pci_alloc_p2pmem")

**Parameters**

`struct pci_dev *pdev`
:   the device the memory was allocated from

`void *addr`
:   address of the memory that was allocated

struct scatterlist \*pci\_p2pmem\_alloc\_sgl(struct pci\_dev \*pdev, unsigned int \*nents, u32 length)
:   allocate peer-to-peer DMA memory in a scatterlist

**Parameters**

`struct pci_dev *pdev`
:   the device to allocate memory from

`unsigned int *nents`
:   the number of SG entries in the list

`u32 length`
:   number of bytes to allocate

**Return**

`NULL` on error or `struct scatterlist` pointer and **nents** on success

void pci\_p2pmem\_free\_sgl(struct pci\_dev \*pdev, struct scatterlist \*sgl)
:   free a scatterlist allocated by [`pci_p2pmem_alloc_sgl()`](#c.pci_p2pmem_alloc_sgl "pci_p2pmem_alloc_sgl")

**Parameters**

`struct pci_dev *pdev`
:   the device to allocate memory from

`struct scatterlist *sgl`
:   the allocated scatterlist

void pci\_p2pmem\_publish(struct pci\_dev \*pdev, bool publish)
:   publish the peer-to-peer DMA memory for use by other devices with `pci_p2pmem_find()`

**Parameters**

`struct pci_dev *pdev`
:   the device with peer-to-peer DMA memory to publish

`bool publish`
:   set to true to publish the memory, false to unpublish it

**Description**

Published memory can be used by other PCI device drivers for
peer-2-peer DMA operations. Non-published memory is reserved for
exclusive use of the device driver that registers the peer-to-peer
memory.

int pci\_p2pdma\_enable\_store(const char \*page, struct pci\_dev \*\*p2p\_dev, bool \*use\_p2pdma)
:   parse a configfs/sysfs attribute store to enable p2pdma

**Parameters**

`const char *page`
:   contents of the value to be stored

`struct pci_dev **p2p_dev`
:   returns the PCI device that was selected to be used
    (if one was specified in the stored value)

`bool *use_p2pdma`
:   returns whether to enable p2pdma or not

**Description**

Parses an attribute value to decide whether to enable p2pdma.
The value can select a PCI device (using its full BDF device
name) or a boolean (in any format [`kstrtobool()`](../../core-api/kernel-api.html#c.kstrtobool "kstrtobool") accepts). A false
value disables p2pdma, a true value expects the caller
to automatically find a compatible device and specifying a PCI device
expects the caller to use the specific provider.

[`pci_p2pdma_enable_show()`](#c.pci_p2pdma_enable_show "pci_p2pdma_enable_show") should be used as the show operation for
the attribute.

Returns 0 on success

ssize\_t pci\_p2pdma\_enable\_show(char \*page, struct pci\_dev \*p2p\_dev, bool use\_p2pdma)
:   show a configfs/sysfs attribute indicating whether p2pdma is enabled

**Parameters**

`char *page`
:   contents of the stored value

`struct pci_dev *p2p_dev`
:   the selected p2p device (NULL if no device is selected)

`bool use_p2pdma`
:   whether p2pdma has been enabled

**Description**

Attributes that use [`pci_p2pdma_enable_store()`](#c.pci_p2pdma_enable_store "pci_p2pdma_enable_store") should use this function
to show the value of the attribute.

Returns 0 on success
