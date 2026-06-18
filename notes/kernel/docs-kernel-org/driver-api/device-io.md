# Bus-Independent Device Accesses

> 출처(원문): https://docs.kernel.org/driver-api/device-io.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Bus-Independent Device Accesses

Author:
:   Matthew Wilcox

Author:
:   Alan Cox

## Introduction

Linux provides an API which abstracts performing IO across all buses
and devices, allowing device drivers to be written independently of bus
type.

## Memory Mapped IO

### Getting Access to the Device

The most widely supported form of IO is memory mapped IO. That is, a
part of the CPU’s address space is interpreted not as accesses to
memory, but as accesses to a device. Some architectures define devices
to be at a fixed address, but most have some method of discovering
devices. The PCI bus walk is a good example of such a scheme. This
document does not cover how to receive such an address, but assumes you
are starting with one. Physical addresses are of type unsigned long.

This address should not be used directly. Instead, to get an address
suitable for passing to the accessor functions described below, you
should call [`ioremap()`](#c.ioremap "ioremap"). An address suitable for accessing
the device will be returned to you.

After you’ve finished using the device (say, in your module’s exit
routine), call `iounmap()` in order to return the address
space to the kernel. Most architectures allocate new address space each
time you call [`ioremap()`](#c.ioremap "ioremap"), and they can run out unless you
call `iounmap()`.

### Accessing the device

The part of the interface most used by drivers is reading and writing
memory-mapped registers on the device. Linux provides interfaces to read
and write 8-bit, 16-bit, 32-bit and 64-bit quantities. Due to a
historical accident, these are named byte, word, long and quad accesses.
Both read and write accesses are supported; there is no prefetch support
at this time.

The functions are named `readb()`, `readw()`, `readl()`, `readq()`,
`readb_relaxed()`, `readw_relaxed()`, `readl_relaxed()`, `readq_relaxed()`,
`writeb()`, `writew()`, `writel()` and `writeq()`.

Some devices (such as framebuffers) would like to use larger transfers than
8 bytes at a time. For these devices, the `memcpy_toio()`,
`memcpy_fromio()` and `memset_io()` functions are
provided. Do not use memset or memcpy on IO addresses; they are not
guaranteed to copy data in order.

The read and write functions are defined to be ordered. That is the
compiler is not permitted to reorder the I/O sequence. When the ordering
can be compiler optimised, you can use `__readb()` and friends to
indicate the relaxed ordering. Use this with care.

While the basic functions are defined to be synchronous with respect to
each other and ordered with respect to each other the buses the devices
sit on may themselves have asynchronicity. In particular many authors
are burned by the fact that PCI bus writes are posted asynchronously. A
driver author must issue a read from the same device to ensure that
writes have occurred in the specific cases the author cares. This kind
of property cannot be hidden from driver writers in the API. In some
cases, the read used to flush the device may be expected to fail (if the
card is resetting, for example). In that case, the read should be done
from config space, which is guaranteed to soft-fail if the card doesn’t
respond.

The following is an example of flushing a write to a device when the
driver would like to ensure the write’s effects are visible prior to
continuing execution:

```
static inline void
qla1280_disable_intrs(struct scsi_qla_host *ha)
{
    struct device_reg *reg;

    reg = ha->iobase;
    /* disable risc and host interrupts */
    WRT_REG_WORD(&reg->ictrl, 0);
    /*
     * The following read will ensure that the above write
     * has been received by the device before we return from this
     * function.
     */
    RD_REG_WORD(&reg->ictrl);
    ha->flags.ints_enabled = 0;
}
```

PCI ordering rules also guarantee that PIO read responses arrive after any
outstanding DMA writes from that bus, since for some devices the result of
a `readb()` call may signal to the driver that a DMA transaction is
complete. In many cases, however, the driver may want to indicate that the
next `readb()` call has no relation to any previous DMA writes
performed by the device. The driver can use `readb_relaxed()` for
these cases, although only some platforms will honor the relaxed
semantics. Using the relaxed read functions will provide significant
performance benefits on platforms that support it. The qla2xxx driver
provides examples of how to use `readX_relaxed()`. In many cases, a majority
of the driver’s `readX()` calls can safely be converted to `readX_relaxed()`
calls, since only a few will indicate or depend on DMA completion.

## Port Space Accesses

### Port Space Explained

Another form of IO commonly supported is Port Space. This is a range of
addresses separate to the normal memory address space. Access to these
addresses is generally not as fast as accesses to the memory mapped
addresses, and it also has a potentially smaller address space.

Unlike memory mapped IO, no preparation is required to access port
space.

### Accessing Port Space

Accesses to this space are provided through a set of functions which
allow 8-bit, 16-bit and 32-bit accesses; also known as byte, word and
long. These functions are `inb()`, `inw()`,
`inl()`, `outb()`, `outw()` and
`outl()`.

Some variants are provided for these functions. Some devices require
that accesses to their ports are slowed down. This functionality is
provided by appending a `_p` to the end of the function.
There are also equivalents to memcpy. The `ins()` and
`outs()` functions copy bytes, words or longs to the given
port.

## \_\_iomem pointer tokens

The data type for an MMIO address is an `__iomem` qualified pointer, such as
`void __iomem *reg`. On most architectures it is a regular pointer that
points to a virtual memory address and can be offset or dereferenced, but in
portable code, it must only be passed from and to functions that explicitly
operated on an `__iomem` token, in particular the [`ioremap()`](#c.ioremap "ioremap") and
`readl()`/`writel()` functions. The ‘sparse’ semantic code checker can be used to
verify that this is done correctly.

While on most architectures, [`ioremap()`](#c.ioremap "ioremap") creates a page table entry for an
uncached virtual address pointing to the physical MMIO address, some
architectures require special instructions for MMIO, and the `__iomem` pointer
just encodes the physical address or an offsettable cookie that is interpreted
by `readl()`/`writel()`.

## Differences between I/O access functions

`readq()`, `readl()`, `readw()`, `readb()`, `writeq()`, `writel()`, `writew()`, `writeb()`

> These are the most generic accessors, providing serialization against other
> MMIO accesses and DMA accesses as well as fixed endianness for accessing
> little-endian PCI devices and on-chip peripherals. Portable device drivers
> should generally use these for any access to `__iomem` pointers.
>
> Note that posted writes are not strictly ordered against a spinlock, see
> [Ordering I/O writes to memory-mapped addresses](io_ordering.html).

`readq_relaxed()`, `readl_relaxed()`, `readw_relaxed()`, `readb_relaxed()`,
`writeq_relaxed()`, `writel_relaxed()`, `writew_relaxed()`, `writeb_relaxed()`

> On architectures that require an expensive barrier for serializing against
> DMA, these “relaxed” versions of the MMIO accessors only serialize against
> each other, but contain a less expensive barrier operation. A device driver
> might use these in a particularly performance sensitive fast path, with a
> comment that explains why the usage in a specific location is safe without
> the extra barriers.
>
> See memory-barriers.txt for a more detailed discussion on the precise ordering
> guarantees of the non-relaxed and relaxed versions.

`ioread64()`, `ioread32()`, `ioread16()`, `ioread8()`,
`iowrite64()`, `iowrite32()`, `iowrite16()`, `iowrite8()`

> These are an alternative to the normal `readl()`/`writel()` functions, with almost
> identical behavior, but they can also operate on `__iomem` tokens returned
> for mapping PCI I/O space with [`pci_iomap()`](pci/pci.html#c.pci_iomap "pci_iomap") or `ioport_map()`. On architectures
> that require special instructions for I/O port access, this adds a small
> overhead for an indirect function call implemented in lib/iomap.c, while on
> other architectures, these are simply aliases.

`ioread64be()`, `ioread32be()`, `ioread16be()`
`iowrite64be()`, `iowrite32be()`, `iowrite16be()`

> These behave in the same way as the `ioread32()`/`iowrite32()` family, but with
> reversed byte order, for accessing devices with big-endian MMIO registers.
> Device drivers that can operate on either big-endian or little-endian
> registers may have to implement a custom wrapper function that picks one or
> the other depending on which device was found.
>
> Note: On some architectures, the normal `readl()`/`writel()` functions
> traditionally assume that devices are the same endianness as the CPU, while
> using a hardware byte-reverse on the PCI bus when running a big-endian kernel.
> Drivers that use `readl()`/`writel()` this way are generally not portable, but
> tend to be limited to a particular SoC.

`hi_lo_readq()`, `lo_hi_readq()`, `hi_lo_readq_relaxed()`, `lo_hi_readq_relaxed()`,
`ioread64_lo_hi()`, `ioread64_hi_lo()`, `ioread64be_lo_hi()`, `ioread64be_hi_lo()`,
`hi_lo_writeq()`, `lo_hi_writeq()`, `hi_lo_writeq_relaxed()`, `lo_hi_writeq_relaxed()`,
`iowrite64_lo_hi()`, `iowrite64_hi_lo()`, `iowrite64be_lo_hi()`, `iowrite64be_hi_lo()`

> Some device drivers have 64-bit registers that cannot be accessed atomically
> on 32-bit architectures but allow two consecutive 32-bit accesses instead.
> Since it depends on the particular device which of the two halves has to be
> accessed first, a helper is provided for each combination of 64-bit accessors
> with either low/high or high/low word ordering. A device driver must include
> either <linux/io-64-nonatomic-lo-hi.h> or <linux/io-64-nonatomic-hi-lo.h> to
> get the function definitions along with helpers that redirect the normal
> `readq()`/`writeq()` to them on architectures that do not provide 64-bit access
> natively.

`__raw_readq()`, `__raw_readl()`, `__raw_readw()`, `__raw_readb()`,
`__raw_writeq()`, `__raw_writel()`, `__raw_writew()`, `__raw_writeb()`

> These are low-level MMIO accessors without barriers or byteorder changes and
> architecture specific behavior. Accesses are usually atomic in the sense that
> a four-byte `__raw_readl()` does not get split into individual byte loads, but
> multiple consecutive accesses can be combined on the bus. In portable code, it
> is only safe to use these to access memory behind a device bus but not MMIO
> registers, as there are no ordering guarantees with regard to other MMIO
> accesses or even spinlocks. The byte order is generally the same as for normal
> memory, so unlike the other functions, these can be used to copy data between
> kernel memory and device memory.

`inl()`, `inw()`, `inb()`, `outl()`, `outw()`, `outb()`

> PCI I/O port resources traditionally require separate helpers as they are
> implemented using special instructions on the x86 architecture. On most other
> architectures, these are mapped to `readl()`/`writel()` style accessors
> internally, usually pointing to a fixed area in virtual memory. Instead of an
> `__iomem` pointer, the address is a 32-bit integer token to identify a port
> number. PCI requires I/O port access to be non-posted, meaning that an `outb()`
> must complete before the following code executes, while a normal `writeb()` may
> still be in progress. On architectures that correctly implement this, I/O port
> access is therefore ordered against spinlocks. Many non-x86 PCI host bridge
> implementations and CPU architectures however fail to implement non-posted I/O
> space on PCI, so they can end up being posted on such hardware.
>
> In some architectures, the I/O port number space has a 1:1 mapping to
> `__iomem` pointers, but this is not recommended and device drivers should
> not rely on that for portability. Similarly, an I/O port number as described
> in a PCI base address register may not correspond to the port number as seen
> by a device driver. Portable drivers need to read the port number for the
> resource provided by the kernel.
>
> There are no direct 64-bit I/O port accessors, but [`pci_iomap()`](pci/pci.html#c.pci_iomap "pci_iomap") in combination
> with ioread64/iowrite64 can be used instead.

`inl_p()`, `inw_p()`, `inb_p()`, `outl_p()`, `outw_p()`, `outb_p()`

> On ISA devices that require specific timing, the \_p versions of the I/O
> accessors add a small delay. On architectures that do not have ISA buses,
> these are aliases to the normal inb/outb helpers.

readsq, readsl, readsw, readsb
writesq, writesl, writesw, writesb
ioread64\_rep, ioread32\_rep, ioread16\_rep, ioread8\_rep
iowrite64\_rep, iowrite32\_rep, iowrite16\_rep, iowrite8\_rep
insl, insw, insb, outsl, outsw, outsb

> These are helpers that access the same address multiple times, usually to copy
> data between kernel memory byte stream and a FIFO buffer. Unlike the normal
> MMIO accessors, these do not perform a byteswap on big-endian kernels, so the
> first byte in the FIFO register corresponds to the first byte in the memory
> buffer regardless of the architecture.

## Device memory mapping modes

Some architectures support multiple modes for mapping device memory.
ioremap\_\*() variants provide a common abstraction around these
architecture-specific modes, with a shared set of semantics.

[`ioremap()`](#c.ioremap "ioremap") is the most common mapping type, and is applicable to typical device
memory (e.g. I/O registers). Other modes can offer weaker or stronger
guarantees, if supported by the architecture. From most to least common, they
are as follows:

### ioremap()

The default mode, suitable for most memory-mapped devices, e.g. control
registers. Memory mapped using [`ioremap()`](#c.ioremap "ioremap") has the following characteristics:

* Uncached - CPU-side caches are bypassed, and all reads and writes are handled
  directly by the device
* No speculative operations - the CPU may not issue a read or write to this
  memory, unless the instruction that does so has been reached in committed
  program flow.
* No reordering - The CPU may not reorder accesses to this memory mapping with
  respect to each other. On some architectures, this relies on barriers in
  `readl_relaxed()`/`writel_relaxed()`.
* No repetition - The CPU may not issue multiple reads or writes for a single
  program instruction.
* No write-combining - Each I/O operation results in one discrete read or write
  being issued to the device, and multiple writes are not combined into larger
  writes. This may or may not be enforced when using \_\_raw I/O accessors or
  pointer dereferences.
* Non-executable - The CPU is not allowed to speculate instruction execution
  from this memory (it probably goes without saying, but you’re also not
  allowed to jump into device memory).

On many platforms and buses (e.g. PCI), writes issued through [`ioremap()`](#c.ioremap "ioremap")
mappings are posted, which means that the CPU does not wait for the write to
actually reach the target device before retiring the write instruction.

On many platforms, I/O accesses must be aligned with respect to the access
size; failure to do so will result in an exception or unpredictable results.

### ioremap\_wc()

Maps I/O memory as normal memory with write combining. Unlike [`ioremap()`](#c.ioremap "ioremap"),

* The CPU may speculatively issue reads from the device that the program
  didn’t actually execute, and may choose to basically read whatever it wants.
* The CPU may reorder operations as long as the result is consistent from the
  program’s point of view.
* The CPU may write to the same location multiple times, even when the program
  issued a single write.
* The CPU may combine several writes into a single larger write.

This mode is typically used for video framebuffers, where it can increase
performance of writes. It can also be used for other blocks of memory in
devices (e.g. buffers or shared memory), but care must be taken as accesses are
not guaranteed to be ordered with respect to normal [`ioremap()`](#c.ioremap "ioremap") MMIO register
accesses without explicit barriers.

On a PCI bus, it is usually safe to use `ioremap_wc()` on MMIO areas marked as
`IORESOURCE_PREFETCH`, but it may not be used on those without the flag.
For on-chip devices, there is no corresponding flag, but a driver can use
`ioremap_wc()` on a device that is known to be safe.

### ioremap\_wt()

Maps I/O memory as normal memory with write-through caching. Like `ioremap_wc()`,
but also,

* The CPU may cache writes issued to and reads from the device, and serve reads
  from that cache.

This mode is sometimes used for video framebuffers, where drivers still expect
writes to reach the device in a timely manner (and not be stuck in the CPU
cache), but reads may be served from the cache for efficiency. However, it is
rarely useful these days, as framebuffer drivers usually perform writes only,
for which `ioremap_wc()` is more efficient (as it doesn’t needlessly trash the
cache). Most drivers should not use this.

### ioremap\_np()

Like [`ioremap()`](#c.ioremap "ioremap"), but explicitly requests non-posted write semantics. On some
architectures and buses, [`ioremap()`](#c.ioremap "ioremap") mappings have posted write semantics, which
means that writes can appear to “complete” from the point of view of the
CPU before the written data actually arrives at the target device. Writes are
still ordered with respect to other writes and reads from the same device, but
due to the posted write semantics, this is not the case with respect to other
devices. `ioremap_np()` explicitly requests non-posted semantics, which means
that the write instruction will not appear to complete until the device has
received (and to some platform-specific extent acknowledged) the written data.

This mapping mode primarily exists to cater for platforms with bus fabrics that
require this particular mapping mode to work correctly. These platforms set the
`IORESOURCE_MEM_NONPOSTED` flag for a resource that requires `ioremap_np()`
semantics and portable drivers should use an abstraction that automatically
selects it where appropriate (see the [Higher-level ioremap abstractions](#higher-level-ioremap-abstractions)
section below).

The bare `ioremap_np()` is only available on some architectures; on others, it
always returns NULL. Drivers should not normally use it, unless they are
platform-specific or they derive benefit from non-posted writes where
supported, and can fall back to [`ioremap()`](#c.ioremap "ioremap") otherwise. The normal approach to
ensure posted write completion is to do a dummy read after a write as
explained in [Accessing the device](#accessing-the-device), which works with [`ioremap()`](#c.ioremap "ioremap") on all
platforms.

`ioremap_np()` should never be used for PCI drivers. PCI memory space writes are
always posted, even on architectures that otherwise implement `ioremap_np()`.
Using `ioremap_np()` for PCI BARs will at best result in posted write semantics,
and at worst result in complete breakage.

Note that non-posted write semantics are orthogonal to CPU-side ordering
guarantees. A CPU may still choose to issue other reads or writes before a
non-posted write instruction retires. See the previous section on MMIO access
functions for details on the CPU side of things.

### ioremap\_uc()

`ioremap_uc()` is only meaningful on old x86-32 systems with the PAT extension,
and on ia64 with its slightly unconventional [`ioremap()`](#c.ioremap "ioremap") behavior, everywhere
elss `ioremap_uc()` defaults to return NULL.

Portable drivers should avoid the use of `ioremap_uc()`, use [`ioremap()`](#c.ioremap "ioremap") instead.

### ioremap\_cache()

`ioremap_cache()` effectively maps I/O memory as normal RAM. CPU write-back
caches can be used, and the CPU is free to treat the device as if it were a
block of RAM. This should never be used for device memory which has side
effects of any kind, or which does not return the data previously written on
read.

It should also not be used for actual RAM, as the returned pointer is an
`__iomem` token. `memremap()` can be used for mapping normal RAM that is outside
of the linear kernel memory area to a regular pointer.

Portable drivers should avoid the use of `ioremap_cache()`.

### Architecture example

Here is how the above modes map to memory attribute settings on the ARM64
architecture:

|  |  |
| --- | --- |
| API | Memory region type and cacheability |
| `ioremap_np()` | Device-nGnRnE |
| [`ioremap()`](#c.ioremap "ioremap") | Device-nGnRE |
| `ioremap_uc()` | (not implemented) |
| `ioremap_wc()` | Normal-Non Cacheable |
| `ioremap_wt()` | (not implemented; fallback to ioremap) |
| `ioremap_cache()` | Normal-Write-Back Cacheable |

## Higher-level ioremap abstractions

Instead of using the above raw [`ioremap()`](#c.ioremap "ioremap") modes, drivers are encouraged to use
higher-level APIs. These APIs may implement platform-specific logic to
automatically choose an appropriate ioremap mode on any given bus, allowing for
a platform-agnostic driver to work on those platforms without any special
cases. At the time of this writing, the following [`ioremap()`](#c.ioremap "ioremap") wrappers have such
logic:

`devm_ioremap_resource()`

> Can automatically select `ioremap_np()` over [`ioremap()`](#c.ioremap "ioremap") according to platform
> requirements, if the `IORESOURCE_MEM_NONPOSTED` flag is set on the `struct
> resource`. Uses devres to automatically unmap the resource when the driver
> `probe()` function fails or a device in unbound from its driver.
>
> Documented in [Devres - Managed Device Resource](driver-model/devres.html).

[`of_address_to_resource()`](../devicetree/kernel-api.html#c.of_address_to_resource "of_address_to_resource")

> Automatically sets the `IORESOURCE_MEM_NONPOSTED` flag for platforms that
> require non-posted writes for certain buses (see the nonposted-mmio and
> posted-mmio device tree properties).

[`of_iomap()`](../devicetree/kernel-api.html#c.of_iomap "of_iomap")

> Maps the resource described in a `reg` property in the device tree, doing
> all required translations. Automatically selects `ioremap_np()` according to
> platform requirements, as above.

`pci_ioremap_bar()`, `pci_ioremap_wc_bar()`

> Maps the resource described in a PCI base address without having to extract
> the physical address first.

[`pci_iomap()`](pci/pci.html#c.pci_iomap "pci_iomap"), [`pci_iomap_wc()`](pci/pci.html#c.pci_iomap_wc "pci_iomap_wc")

> Like `pci_ioremap_bar()`/`pci_ioremap_bar()`, but also works on I/O space when
> used together with `ioread32()`/`iowrite32()` and similar accessors

[`pcim_iomap()`](pci/pci.html#c.pcim_iomap "pcim_iomap")

> Like [`pci_iomap()`](pci/pci.html#c.pci_iomap "pci_iomap"), but uses devres to automatically unmap the resource when
> the driver `probe()` function fails or a device in unbound from its driver
>
> Documented in [Devres - Managed Device Resource](driver-model/devres.html).

Not using these wrappers may make drivers unusable on certain platforms with
stricter rules for mapping I/O memory.

## Generalizing Access to System and I/O Memory

When accessing a memory region, depending on its location, users may have to
access it with I/O operations or memory load/store operations. For example,
copying to system memory could be done with [`memcpy()`](../core-api/kernel-api.html#c.memcpy "memcpy"), copying to I/O memory
would be done with `memcpy_toio()`.

```
void *vaddr = ...; // pointer to system memory
memcpy(vaddr, src, len);

void *vaddr_iomem = ...; // pointer to I/O memory
memcpy_toio(vaddr_iomem, src, len);
```

The user of such pointer may not have information about the mapping of that
region or may want to have a single code path to handle operations on that
buffer, regardless if it’s located in system or IO memory. The type
[`struct iosys_map`](#c.iosys_map "iosys_map") and its helpers abstract that so the
buffer can be passed around to other drivers or have separate duties inside
the same driver for allocation, read and write operations.

Open-coding access to [`struct iosys_map`](#c.iosys_map "iosys_map") is considered
bad style. Rather than accessing its fields directly, use one of the provided
helper functions, or implement your own. For example, instances of
[`struct iosys_map`](#c.iosys_map "iosys_map") can be initialized statically with
[`IOSYS_MAP_INIT_VADDR()`](#c.IOSYS_MAP_INIT_VADDR "IOSYS_MAP_INIT_VADDR"), or at runtime with [`iosys_map_set_vaddr()`](#c.iosys_map_set_vaddr "iosys_map_set_vaddr"). These
helpers will set an address in system memory.

```
struct iosys_map map = IOSYS_MAP_INIT_VADDR(0xdeadbeaf);

iosys_map_set_vaddr(&map, 0xdeadbeaf);
```

To set an address in I/O memory, use [`IOSYS_MAP_INIT_VADDR_IOMEM()`](#c.IOSYS_MAP_INIT_VADDR_IOMEM "IOSYS_MAP_INIT_VADDR_IOMEM") or
[`iosys_map_set_vaddr_iomem()`](#c.iosys_map_set_vaddr_iomem "iosys_map_set_vaddr_iomem").

```
struct iosys_map map = IOSYS_MAP_INIT_VADDR_IOMEM(0xdeadbeaf);

iosys_map_set_vaddr_iomem(&map, 0xdeadbeaf);
```

Instances of [`struct iosys_map`](#c.iosys_map "iosys_map") do not have to be cleaned up, but
can be cleared to NULL with [`iosys_map_clear()`](#c.iosys_map_clear "iosys_map_clear"). Cleared mappings
always refer to system memory.

```
iosys_map_clear(&map);
```

Test if a mapping is valid with either [`iosys_map_is_set()`](#c.iosys_map_is_set "iosys_map_is_set") or
[`iosys_map_is_null()`](#c.iosys_map_is_null "iosys_map_is_null").

```
if (iosys_map_is_set(&map) != iosys_map_is_null(&map))
        // always true
```

Instances of [`struct iosys_map`](#c.iosys_map "iosys_map") can be compared for
equality with [`iosys_map_is_equal()`](#c.iosys_map_is_equal "iosys_map_is_equal"). Mappings that point to different memory
spaces, system or I/O, are never equal. That’s even true if both spaces are
located in the same address space, both mappings contain the same address
value, or both mappings refer to NULL.

```
struct iosys_map sys_map; // refers to system memory
struct iosys_map io_map; // refers to I/O memory

if (iosys_map_is_equal(&sys_map, &io_map))
        // always false
```

A set up instance of [`struct iosys_map`](#c.iosys_map "iosys_map") can be used to access or manipulate the
buffer memory. Depending on the location of the memory, the provided helpers
will pick the correct operations. Data can be copied into the memory with
[`iosys_map_memcpy_to()`](#c.iosys_map_memcpy_to "iosys_map_memcpy_to"). The address can be manipulated with [`iosys_map_incr()`](#c.iosys_map_incr "iosys_map_incr").

```
const void *src = ...; // source buffer
size_t len = ...; // length of src

iosys_map_memcpy_to(&map, src, len);
iosys_map_incr(&map, len); // go to first byte after the memcpy
```

struct iosys\_map
:   Pointer to IO/system memory

**Definition**:

```
struct iosys_map {
    union {
        void __iomem *vaddr_iomem;
        void *vaddr;
    };
    bool is_iomem;
};
```

**Members**

`{unnamed_union}`
:   anonymous

`vaddr_iomem`
:   The buffer’s address if in I/O memory

`vaddr`
:   The buffer’s address if in system memory

`is_iomem`
:   True if the buffer is located in I/O memory, or false
    otherwise.

IOSYS\_MAP\_INIT\_VADDR

`IOSYS_MAP_INIT_VADDR (vaddr_)`

> Initializes [`struct iosys_map`](#c.iosys_map "iosys_map") to an address in system memory

**Parameters**

`vaddr_`
:   A system-memory address

IOSYS\_MAP\_INIT\_VADDR\_IOMEM

`IOSYS_MAP_INIT_VADDR_IOMEM (vaddr_iomem_)`

> Initializes [`struct iosys_map`](#c.iosys_map "iosys_map") to an address in I/O memory

**Parameters**

`vaddr_iomem_`
:   An I/O-memory address

IOSYS\_MAP\_INIT\_OFFSET

`IOSYS_MAP_INIT_OFFSET (map_, offset_)`

> Initializes [`struct iosys_map`](#c.iosys_map "iosys_map") from another iosys\_map

**Parameters**

`map_`
:   The dma-buf mapping structure to copy from

`offset_`
:   Offset to add to the other mapping

**Description**

Initializes a new iosys\_map `struct based` on another passed as argument. It
does a shallow copy of the `struct so` it’s possible to update the back storage
without changing where the original map points to. It is the equivalent of
doing:

```
iosys_map map = other_map;
iosys_map_incr(&map, &offset);
```

Example usage:

```
void foo(struct device *dev, struct iosys_map *base_map)
{
        ...
        struct iosys_map map = IOSYS_MAP_INIT_OFFSET(base_map, FIELD_OFFSET);
        ...
}
```

The advantage of using the initializer over just increasing the offset with
[`iosys_map_incr()`](#c.iosys_map_incr "iosys_map_incr") like above is that the new map will always point to the
right place of the buffer during its scope. It reduces the risk of updating
the wrong part of the buffer and having no compiler warning about that. If
the assignment to [`IOSYS_MAP_INIT_OFFSET()`](#c.IOSYS_MAP_INIT_OFFSET "IOSYS_MAP_INIT_OFFSET") is forgotten, the compiler can warn
about the use of uninitialized variable.

void iosys\_map\_set\_vaddr(struct [iosys\_map](#c.iosys_map "iosys_map") \*map, void \*vaddr)
:   Sets a iosys mapping structure to an address in system memory

**Parameters**

`struct iosys_map *map`
:   The iosys\_map structure

`void *vaddr`
:   A system-memory address

**Description**

Sets the address and clears the I/O-memory flag.

void iosys\_map\_set\_vaddr\_iomem(struct [iosys\_map](#c.iosys_map "iosys_map") \*map, void \_\_iomem \*vaddr\_iomem)
:   Sets a iosys mapping structure to an address in I/O memory

**Parameters**

`struct iosys_map *map`
:   The iosys\_map structure

`void __iomem *vaddr_iomem`
:   An I/O-memory address

**Description**

Sets the address and the I/O-memory flag.

bool iosys\_map\_is\_equal(const struct [iosys\_map](#c.iosys_map "iosys_map") \*lhs, const struct [iosys\_map](#c.iosys_map "iosys_map") \*rhs)
:   Compares two iosys mapping structures for equality

**Parameters**

`const struct iosys_map *lhs`
:   The iosys\_map structure

`const struct iosys_map *rhs`
:   A iosys\_map structure to compare with

**Description**

Two iosys mapping structures are equal if they both refer to the same type of memory
and to the same address within that memory.

**Return**

True is both structures are equal, or false otherwise.

bool iosys\_map\_is\_null(const struct [iosys\_map](#c.iosys_map "iosys_map") \*map)
:   Tests for a iosys mapping to be NULL

**Parameters**

`const struct iosys_map *map`
:   The iosys\_map structure

**Description**

Depending on the state of [`struct iosys_map`](#c.iosys_map "iosys_map").is\_iomem, tests if the
mapping is NULL.

**Return**

True if the mapping is NULL, or false otherwise.

bool iosys\_map\_is\_set(const struct [iosys\_map](#c.iosys_map "iosys_map") \*map)
:   Tests if the iosys mapping has been set

**Parameters**

`const struct iosys_map *map`
:   The iosys\_map structure

**Description**

Depending on the state of [`struct iosys_map`](#c.iosys_map "iosys_map").is\_iomem, tests if the
mapping has been set.

**Return**

True if the mapping is been set, or false otherwise.

void iosys\_map\_clear(struct [iosys\_map](#c.iosys_map "iosys_map") \*map)
:   Clears a iosys mapping structure

**Parameters**

`struct iosys_map *map`
:   The iosys\_map structure

**Description**

Clears all fields to zero, including [`struct iosys_map`](#c.iosys_map "iosys_map").is\_iomem, so
mapping structures that were set to point to I/O memory are reset for
system memory. Pointers are cleared to NULL. This is the default.

void iosys\_map\_memcpy\_to(struct [iosys\_map](#c.iosys_map "iosys_map") \*dst, size\_t dst\_offset, const void \*src, size\_t len)
:   Memcpy into offset of iosys\_map

**Parameters**

`struct iosys_map *dst`
:   The iosys\_map structure

`size_t dst_offset`
:   The offset from which to copy

`const void *src`
:   The source buffer

`size_t len`
:   The number of byte in src

**Description**

Copies data into a iosys\_map with an offset. The source buffer is in
system memory. Depending on the buffer’s location, the helper picks the
correct method of accessing the memory.

void iosys\_map\_memcpy\_from(void \*dst, const struct [iosys\_map](#c.iosys_map "iosys_map") \*src, size\_t src\_offset, size\_t len)
:   Memcpy from iosys\_map into system memory

**Parameters**

`void *dst`
:   Destination in system memory

`const struct iosys_map *src`
:   The iosys\_map structure

`size_t src_offset`
:   The offset from which to copy

`size_t len`
:   The number of byte in src

**Description**

Copies data from a iosys\_map with an offset. The dest buffer is in
system memory. Depending on the mapping location, the helper picks the
correct method of accessing the memory.

void iosys\_map\_incr(struct [iosys\_map](#c.iosys_map "iosys_map") \*map, size\_t incr)
:   Increments the address stored in a iosys mapping

**Parameters**

`struct iosys_map *map`
:   The iosys\_map structure

`size_t incr`
:   The number of bytes to increment

**Description**

Increments the address stored in a iosys mapping. Depending on the
buffer’s location, the correct value will be updated.

void iosys\_map\_memset(struct [iosys\_map](#c.iosys_map "iosys_map") \*dst, size\_t offset, int value, size\_t len)
:   Memset iosys\_map

**Parameters**

`struct iosys_map *dst`
:   The iosys\_map structure

`size_t offset`
:   Offset from dst where to start setting value

`int value`
:   The value to set

`size_t len`
:   The number of bytes to set in dst

**Description**

Set value in iosys\_map. Depending on the buffer’s location, the helper
picks the correct method of accessing the memory.

iosys\_map\_rd

`iosys_map_rd (map__, offset__, type__)`

> Read a C-type value from the iosys\_map

**Parameters**

`map__`
:   The iosys\_map structure

`offset__`
:   The offset from which to read

`type__`
:   Type of the value being read

**Description**

Read a C type value (u8, u16, u32 and u64) from iosys\_map. For other types or
if pointer may be unaligned (and problematic for the architecture supported),
use [`iosys_map_memcpy_from()`](#c.iosys_map_memcpy_from "iosys_map_memcpy_from").

**Return**

The value read from the mapping.

iosys\_map\_wr

`iosys_map_wr (map__, offset__, type__, val__)`

> Write a C-type value to the iosys\_map

**Parameters**

`map__`
:   The iosys\_map structure

`offset__`
:   The offset from the mapping to write to

`type__`
:   Type of the value being written

`val__`
:   Value to write

**Description**

Write a C type value (u8, u16, u32 and u64) to the iosys\_map. For other types
or if pointer may be unaligned (and problematic for the architecture
supported), use [`iosys_map_memcpy_to()`](#c.iosys_map_memcpy_to "iosys_map_memcpy_to")

iosys\_map\_rd\_field

`iosys_map_rd_field (map__, struct_offset__, struct_type__, field__)`

> Read a member from a `struct in` the iosys\_map

**Parameters**

`map__`
:   The iosys\_map structure

`struct_offset__`
:   Offset from the beginning of the map, where the `struct
    is` located

`struct_type__`
:   The `struct describing` the layout of the mapping

`field__`
:   Member of the `struct to` read

**Description**

Read a value from iosys\_map considering its layout is described by a C `struct
starting` at **struct\_offset\_\_**. The field offset and size is calculated and its
value read. If the field access would incur in un-aligned access, then either
[`iosys_map_memcpy_from()`](#c.iosys_map_memcpy_from "iosys_map_memcpy_from") needs to be used or the architecture must support it.
For example: suppose there is a **struct** foo defined as below and the value
`foo.field2.inner2` needs to be read from the iosys\_map:

```
struct foo {
        int field1;
        struct {
                int inner1;
                int inner2;
        } field2;
        int field3;
} __packed;
```

This is the expected memory layout of a buffer using [`iosys_map_rd_field()`](#c.iosys_map_rd_field "iosys_map_rd_field"):

| Address | Content |
| --- | --- |
| buffer + 0000 | start of mmapped buffer pointed by iosys\_map |
| ... | ... |
| buffer + `struct_offset__` | start of `struct foo` |
| ... | ... |
| buffer + wwww | `foo.field2.inner2` |
| ... | ... |
| buffer + yyyy | end of `struct foo` |
| ... | ... |
| buffer + zzzz | end of mmaped buffer |

Values automatically calculated by this macro or not needed are denoted by
wwww, yyyy and zzzz. This is the code to read that value:

```
x = iosys_map_rd_field(&map, offset, struct foo, field2.inner2);
```

**Return**

The value read from the mapping.

iosys\_map\_wr\_field

`iosys_map_wr_field (map__, struct_offset__, struct_type__, field__, val__)`

> Write to a member of a `struct in` the iosys\_map

**Parameters**

`map__`
:   The iosys\_map structure

`struct_offset__`
:   Offset from the beginning of the map, where the `struct
    is` located

`struct_type__`
:   The `struct describing` the layout of the mapping

`field__`
:   Member of the `struct to` read

`val__`
:   Value to write

**Description**

Write a value to the iosys\_map considering its layout is described by a C
`struct starting` at **struct\_offset\_\_**. The field offset and size is calculated
and the **val\_\_** is written. If the field access would incur in un-aligned
access, then either [`iosys_map_memcpy_to()`](#c.iosys_map_memcpy_to "iosys_map_memcpy_to") needs to be used or the
architecture must support it. Refer to [`iosys_map_rd_field()`](#c.iosys_map_rd_field "iosys_map_rd_field") for expected
usage and memory layout.

## Public Functions Provided

phys\_addr\_t virt\_to\_phys(volatile void \*address)
:   map virtual addresses to physical

**Parameters**

`volatile void *address`
:   address to remap

**Description**

> The returned physical address is the physical (CPU) mapping for
> the memory address given. It is only valid to use this function on
> addresses directly mapped or allocated via kmalloc.
>
> This function does not give bus mappings for DMA transfers. In
> almost all conceivable cases a device driver should not be using
> this function

void \*phys\_to\_virt(phys\_addr\_t address)
:   map physical address to virtual

**Parameters**

`phys_addr_t address`
:   address to remap

**Description**

> The returned virtual address is a current CPU mapping for
> the memory address given. It is only valid to use this function on
> addresses that have a kernel mapping
>
> This function does not handle bus mappings for DMA transfers. In
> almost all conceivable cases a device driver should not be using
> this function

void \_\_iomem \*ioremap(resource\_size\_t offset, unsigned long size)
:   map bus memory into CPU space

**Parameters**

`resource_size_t offset`
:   bus address of the memory

`unsigned long size`
:   size of the resource to map

**Description**

ioremap performs a platform specific sequence of operations to
make bus memory CPU accessible via the readb/readw/readl/writeb/
writew/writel functions and the other mmio helpers. The returned
address is not guaranteed to be usable directly as a virtual
address.

If the area you are trying to map is a PCI BAR you should have a
look at [`pci_iomap()`](pci/pci.html#c.pci_iomap "pci_iomap").

void iosubmit\_cmds512(void \_\_iomem \*dst, const void \*src, size\_t count)
:   copy data to single MMIO location, in 512-bit units

**Parameters**

`void __iomem *dst`
:   destination, in MMIO space (must be 512-bit aligned)

`const void *src`
:   source

`size_t count`
:   number of 512 bits quantities to submit

**Description**

Submit data from kernel space to MMIO space, in units of 512 bits at a
time. Order of access is not guaranteed, nor is a memory barrier
performed afterwards.

Warning: Do not use this helper unless your driver has checked that the CPU
instruction is supported on the platform.
